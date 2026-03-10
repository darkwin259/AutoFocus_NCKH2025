/*
 * ms41929.c
 *
 * Driver điều khiển IC MS41929 (tương thích MS41908) cho AutoFocus NCKH 2025
 * Platform: STM32F4xx HAL
 *
 * ============================================================
 * NGUYÊN TẮC SPI CỦA MS41929 (từ demo 41908 - bsp_SPI.c):
 * ============================================================
 *  void Spi_Write(u8 addr, unsigned int data) {
 *      CS_High();              // CS HIGH = bắt đầu (latch cũ / deselect)
 *      SPI_SendByte(addr);     // Gửi địa chỉ (8 bit)
 *      SPI_SendByte(data%256); // Gửi data byte thấp (LSB)
 *      SPI_SendByte(data/256); // Gửi data byte cao (MSB)
 *      CS_Low();               // CS LOW = kết thúc (latch dữ liệu mới vào IC)
 *  }
 *
 *  => CS là Active FALLING EDGE: HIGH → gửi → LOW để IC chốt dữ liệu
 *  => SPI Mode 3 (CPOL=High, CPHA=2Edge), LSB First
 *
 * ============================================================
 * RESET (từ bsp_41908.c - hàm MS_Rest):
 * ============================================================
 *  MS_High() -> nop -> MS_Low() -> nop -> MS_High()
 *  (RSTB: HIGH -> LOW -> HIGH, pulse LOW = reset)
 *
 * ============================================================
 * VD_FZ PULSE (từ bsp_SPI.c - hàm VD_FZ):
 * ============================================================
 *  VD_High() -> Delay(200) -> VD_Low()
 *  (Tạo 1 xung HIGH để motor bước 1 bước)
 *
 * ============================================================
 * CẤU HÌNH THANH GHI KHỞI TẠO (từ bsp_41908.c - hàm MS_Int1):
 * ============================================================
 *  Spi_Write(0x20, 0x1e01);  // PWM freq = DT1 = 0.3ms
 *  Spi_Write(0x22, 0x0001);  // DT2 = 0.3ms
 *  Spi_Write(0x23, 0xd8d8);  // Giới hạn dòng AB = 90%
 *  Spi_Write(0x24, 0x0d20);  // LED_B sáng + bước AB
 *  Spi_Write(0x25, 0x07ff);  // Cường độ bước AB = max
 *  Spi_Write(0x27, 0x0002);  // DT2 cho CD
 *  Spi_Write(0x28, 0xd8d8);  // Giới hạn dòng CD = 90%
 *  Spi_Write(0x29, 0x0d20);  // LED_A sáng + bước CD
 *  Spi_Write(0x2a, 0x07ff);  // Cường độ bước CD = max
 *  Spi_Write(0x21, 0x0087);  // Enable PLS1 và PLS2
 *  Spi_Write(0x0b, 0x0480);  // Kích hoạt TESTEN
 *
 * ============================================================
 * ĐIỀU KHIỂN LED (từ stm32f10x_it.c - demo):
 * ============================================================
 *  LED[0] = 0x0500 => LED TẮT  (byte cao = 0x05)
 *  LED[1] = 0x0D00 => LED SÁNG (byte cao = 0x0D)
 *  LED_A: Reg 0x29 bit[15:8] = 0x0D (sáng) hoặc 0x05 (tắt)
 *  LED_B: Reg 0x24 bit[15:8] = 0x0D (sáng) hoặc 0x05 (tắt)
 *
 * ============================================================
 * THAY ĐỔI TỐC ĐỘ / VÒNG (từ stm32f10x_it.c - demo):
 * ============================================================
 *  Speed[]         = {0xFF00, 0x248D, 0x0B16, ...} ghi vào 0x25 / 0x2A
 *  Circle_number[] = {0x01, 0x07, 0x17, ...}       ghi vào byte thấp 0x24 / 0x29
 */

#include "ms41929.h"
#include "main.h"
#include "spi.h"
#include "stm32f4xx_hal.h"

/* ======================================================
 * Biến nội bộ (shadow registers)
 * ====================================================== */
/*
 * Shadow register cho STEP_AB (0x24):
 * Bit [15:8] = {LED_B_ctrl, số_vòng}
 * Bit [7:0]  = byte điều khiển bước thực tế
 *
 * Giá trị mặc định: 0x0D20 => LED_B bật, 0x20 bước
 */
static uint16_t reg_step_ab_shadow = 0x0D20;

/*
 * Shadow register cho STEP_CD (0x29):
 * Bit [15:8] = {LED_A_ctrl, số_vòng}
 * Bit [7:0]  = byte điều khiển bước thực tế
 *
 * Giá trị mặc định: 0x0D20 => LED_A bật, 0x20 bước
 */
static uint16_t reg_step_cd_shadow = 0x0D20;

/*
 * Hướng di chuyển hiện tại:
 * Trong MS41929, hướng được điều khiển bằng cách thay đổi thứ tự bước
 * (tăng/giảm giá trị byte thấp của STEP_AB và STEP_CD)
 * 1 = thuận (CW), 0 = nghịch (CCW)
 */
static uint8_t current_dir = MS_DIR_FORWARD;

/* ======================================================
 * Hàm nội bộ
 * ====================================================== */

/**
 * @brief Ghi dữ liệu vào một thanh ghi của MS41929 qua SPI
 *
 * Theo demo 41908 (bsp_SPI.c):
 *   CS_High() -> Send addr -> Send data_lo -> Send data_hi -> CS_Low()
 *
 * Lưu ý: CS_HIGH = bắt đầu chuỗi truyền (deselect/idle state)
 *         CS_LOW  = kết thúc, IC chốt dữ liệu (latch on falling edge)
 *
 * @param addr  Địa chỉ thanh ghi (8-bit)
 * @param data  Dữ liệu 16-bit cần ghi (LSB trước, sau đó MSB)
 */
void MS41929_WriteReg(uint8_t addr, uint16_t data)
{
    uint8_t pData[3];

    /* Byte 0: địa chỉ thanh ghi */
    pData[0] = addr;

    /* Byte 1: Data byte thấp (data % 256 = data & 0xFF) */
    pData[1] = (uint8_t)(data & 0x00FF);

    /* Byte 2: Data byte cao (data / 256 = data >> 8) */
    pData[2] = (uint8_t)((data >> 8) & 0xFF);

    /*
     * CS_High() trước khi gửi — theo demo: "CS_Hihg() -> SPI_SendByte..."
     * CS_GPIO_Port là GPIOA, MS_CS_Pin là GPIO_PIN_4 (định nghĩa trong main.h)
     */
    HAL_GPIO_WritePin(MS_CS_GPIO_Port, MS_CS_Pin, GPIO_PIN_SET);   /* CS HIGH = bắt đầu */

    /* Gửi 3 bytes qua SPI1 */
    HAL_SPI_Transmit(&hspi1, pData, 3, 100);

    /*
     * CS_Low() sau khi gửi xong — theo demo: "...CS_Low()"
     * IC sẽ chốt dữ liệu vào thanh ghi tại sườn FALLING của CS
     */
    HAL_GPIO_WritePin(MS_CS_GPIO_Port, MS_CS_Pin, GPIO_PIN_RESET); /* CS LOW = kết thúc, latch */
}

/* ======================================================
 * MS41929_Init - Khởi tạo chip
 * Sao chép y hệt MS_Rest() + MS_Int1() từ demo 41908
 * ====================================================== */
void MS41929_Init(void)
{
    /*
     * === BƯỚC 1: RESET CHIP (MS_Rest) ===
     * Từ demo: MS_Hihg() -> nop -> MS_Low() -> nop -> MS_Hihg()
     * RSTB: HIGH -> LOW -> HIGH
     * (RSTB = active LOW: kéo LOW để reset, sau đó HIGH để giải phóng)
     *
     * MS_RSTB_GPIO_Port = GPIOB, MS_RSTB_Pin = GPIO_PIN_1 (trong main.h)
     */
    HAL_GPIO_WritePin(MS_RSTB_GPIO_Port, MS_RSTB_Pin, GPIO_PIN_SET);  /* RSTB = HIGH */
    HAL_Delay(5);                                                       /* Chờ ổn định */

    HAL_GPIO_WritePin(MS_RSTB_GPIO_Port, MS_RSTB_Pin, GPIO_PIN_RESET); /* RSTB = LOW (reset) */
    HAL_Delay(5);                                                        /* Giữ LOW tối thiểu */

    HAL_GPIO_WritePin(MS_RSTB_GPIO_Port, MS_RSTB_Pin, GPIO_PIN_SET);  /* RSTB = HIGH (release) */
    HAL_Delay(10);                                                       /* Chờ IC khởi động */

    /*
     * === BƯỚC 2: CẤU HÌNH THANH GHI (MS_Int1) ===
     * Sao chép y hệt Spi_Write() từ bsp_41908.c - hàm MS_Int1()
     */

    /* 0x20: Cấu hình PWM frequency và Dead Time 1 = 0.3ms */
    MS41929_WriteReg(0x20, 0x1E01);

    /* 0x22: Dead Time 2 cho cuộn AB = 0.3ms */
    MS41929_WriteReg(0x22, 0x0001);

    /* 0x23: Giới hạn dòng điện cuộn AB = 90% (0xD8 cho mỗi byte) */
    MS41929_WriteReg(0x23, 0xD8D8);

    /* 0x24: STEP_AB - LED_B bật + số vòng + bước AB ban đầu */
    MS41929_WriteReg(0x24, reg_step_ab_shadow);  /* 0x0D20 = LED_B bật, vòng 0x0D, bước 0x20 */

    /* 0x25: INTCT_AB - Cường độ dòng bước AB = tối đa (0x07FF) */
    MS41929_WriteReg(0x25, 0x07FF);

    /* 0x27: Dead Time 2 cho cuộn CD = 0.6ms (0x0002) */
    MS41929_WriteReg(0x27, 0x0002);

    /* 0x28: Giới hạn dòng điện cuộn CD = 90% */
    MS41929_WriteReg(0x28, 0xD8D8);

    /* 0x29: STEP_CD - LED_A bật + số vòng + bước CD ban đầu */
    MS41929_WriteReg(0x29, reg_step_cd_shadow);  /* 0x0D20 = LED_A bật, vòng 0x0D, bước 0x20 */

    /* 0x2A: INTCT_CD - Cường độ dòng bước CD = tối đa (0x07FF) */
    MS41929_WriteReg(0x2A, 0x07FF);

    /* 0x21: ĐIỀU KHIỂN - Enable PLS1 và PLS2 (0x0087) */
    /* Đây là thanh ghi quan trọng: bật đầu ra PLS để xung VD_FZ có tác dụng */
    MS41929_WriteReg(0x21, 0x0087);

    /* 0x0B: Kích hoạt TESTEN1 để cho phép test */
    MS41929_WriteReg(0x0B, 0x0480);
}

/* ======================================================
 * MS41929_Pulse - Tạo 1 xung VD_FZ (motor bước 1 bước)
 * Sao chép hàm VD_FZ() từ demo 41908 (bsp_SPI.c):
 *   VD_Hihg() -> Delay(200) -> VD_Low()
 * ====================================================== */
void MS41929_Pulse(void)
{
    /*
     * VD_FZ: PB0 (GPIO_PIN_0 trên GPIOB)
     * Tạo xung HIGH để IC nhận diện 1 bước mới
     * Demo dùng Delay(200) với vòng lặp ~72MHz → khoảng 200 * 200 / 72MHz ≈ 556µs
     * Với STM32F4 100MHz, dùng HAL_Delay(1) ≈ 1ms là an toàn
     */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);   /* VD_FZ = HIGH */
    HAL_Delay(1);                                          /* Giữ HIGH ít nhất 0.5ms */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); /* VD_FZ = LOW */
}

/* ======================================================
 * MS41929_SetDirection - Đặt hướng di chuyển motor
 *
 * Từ Datasheet trang 18:
 *   CCWCWAB (bit D8 của thanh ghi 24h) = hường motor alpha (AB)
 *   CCWCWCD (bit D8 của thanh ghi 29h) = hường motor beta (CD)
 *   D8 = 0 → CW  (thuận chiều kim đồng hồ)
 *   D8 = 1 → CCW (ngược chiều kim đồng hồ)
 *
 * Hàm này cập nhật bit CCWCW trong shadow register và
 * ghi ngay vào IC (phải ghi trước khi gọi MS41929_Step!).
 * ====================================================== */
void MS41929_SetDirection(uint8_t dir)
{
    current_dir = (dir != 0) ? MS_DIR_FORWARD : MS_DIR_BACKWARD;

    /*
     * Theo datasheet:
     *   CCWCWAB = Bit D8 của thanh ghi 24h
     *   CCWCWCD = Bit D8 của thanh ghi 29h
     *
     * Từ demo 41908: khởng định hướng bằng cách ghi lại thanh ghi
     * với byte cao thích hợp.
     *
     * D8=0 (CW):  Byte cao & 0xFE (xóa bit0 của byte cao)
     * D8=1 (CCW): Byte cao | 0x01 (set bit0 của byte cao)
     *
     * Lĩu ý: Byte cao của data = bits D15:D8
     *   Bit D8 = bit0 của byte cao
     *   Byte cao = (reg_step_xx_shadow >> 8) & 0xFF
     */
    if (current_dir == MS_DIR_FORWARD)
    {
        /* CW: Xóa bit D8 (CCWCW = 0) */
        reg_step_ab_shadow &= ~(1 << 8);
        reg_step_cd_shadow &= ~(1 << 8);
    }
    else
    {
        /* CCW: Set bit D8 (CCWCW = 1) */
        reg_step_ab_shadow |= (1 << 8);
        reg_step_cd_shadow |= (1 << 8);
    }

    /* Ghi ngay vào IC — phải thực hiện trước khi gọi Pulse! */
    MS41929_WriteReg(0x24, reg_step_ab_shadow);
    MS41929_WriteReg(0x25, 0x07FF);            /* Giữ cường độ AB */

    MS41929_WriteReg(0x29, reg_step_cd_shadow);
    MS41929_WriteReg(0x2A, 0x07FF);            /* Giữ cường độ CD */
}

/* ======================================================
 * MS41929_Step - Di chuyển motor một số bước xác định
 *
 * Cách hoạt động (theo Datasheet trang 18):
 *   1. PSUMAB[7:0] đã được set trong thanh ghi 24h (từ init: 0x20 = 32 bước)
 *   2. CCWCWAB (bit D8) đã được set bởi SetDirection()
 *   3. Mỗi lần gọi MS41929_Pulse() = tạo 1 xung VD
 *   4. IC sẽ thực hiện PSUMAB bước cho mỗi xung VD
 *
 * @param steps     Số xung VD cần tạo (mỗi xung = PSUMAB bước vật lý)
 * @param speed_ms  Delay (ms) giữa các xung. Giả sử PSUMAB=32:
 *                  2ms delay = 500 xung/giây = 16000 bước/giây
 * ====================================================== */
void MS41929_Step(uint16_t steps, uint16_t speed_ms)
{
    if (steps == 0) return;
    if (speed_ms < 1) speed_ms = 1; /* Tối thiểu 1ms giữa các xung */

    for (uint16_t i = 0; i < steps; i++)
    {
        MS41929_Pulse();   /* Tạo 1 xung VD_FZ */
        HAL_Delay(speed_ms); /* Delay để IC hoàn thành bước đi */
    }
}

/* ======================================================
 * MS41929_ControlLED - Điều khiển LED A và LED B
 *
 * Từ demo (stm32f10x_it.c):
 *   LED[0] = 0x0500 => LED TẮT  (byte cao của STEP_xx = 0x05)
 *   LED[1] = 0x0D00 => LED SÁNG (byte cao của STEP_xx = 0x0D)
 *
 *   Hàm LED_Change_A trong demo:
 *   LED_data = (*dd + *cc)
 *   => LED_data = LED[led_state] + Circle_number[Aperture]
 *   => byte cao: 0x0D hoặc 0x05
 *   => byte thấp: số vòng từ Circle_number[]
 *
 * Ở đây ta giữ nguyên byte thấp (số vòng) và chỉ thay byte cao:
 *   - LED sáng: byte cao = 0x0D → data = 0x0D00 | (byte_thấp_hiện_tại)
 *   - LED tắt:  byte cao = 0x05 → data = 0x0500 | (byte_thấp_hiện_tại)
 *
 * LED_A điều khiển qua STEP_CD (0x29)
 * LED_B điều khiển qua STEP_AB (0x24)
 *
 * @param led_a_state  1 = LED A sáng, 0 = LED A tắt
 * @param led_b_state  1 = LED B sáng, 0 = LED B tắt
 * ====================================================== */
void MS41929_ControlLED(uint8_t led_a_state, uint8_t led_b_state)
{
    /* Byte thấp hiện tại (số vòng / bước) của mỗi kênh */
    uint8_t step_ab_lo = (uint8_t)(reg_step_ab_shadow & 0x00FF);
    uint8_t step_cd_lo = (uint8_t)(reg_step_cd_shadow & 0x00FF);

    /* Cập nhật LED_B (qua STEP_AB - 0x24) */
    if (led_b_state)
        reg_step_ab_shadow = 0x0D00 | step_ab_lo;  /* LED_B bật: byte cao = 0x0D */
    else
        reg_step_ab_shadow = 0x0500 | step_ab_lo;  /* LED_B tắt: byte cao = 0x05 */

    /* Cập nhật LED_A (qua STEP_CD - 0x29) */
    if (led_a_state)
        reg_step_cd_shadow = 0x0D00 | step_cd_lo;  /* LED_A bật: byte cao = 0x0D */
    else
        reg_step_cd_shadow = 0x0500 | step_cd_lo;  /* LED_A tắt: byte cao = 0x05 */

    /* Ghi ra IC — cần ghi cả INTCT sau STEP để dòng điện có hiệu lực */
    MS41929_WriteReg(0x24, reg_step_ab_shadow); /* Cập nhật LED_B */
    MS41929_WriteReg(0x25, 0x07FF);             /* Giữ nguyên cường độ AB */

    MS41929_WriteReg(0x29, reg_step_cd_shadow); /* Cập nhật LED_A */
    MS41929_WriteReg(0x2A, 0x07FF);             /* Giữ nguyên cường độ CD */
}
