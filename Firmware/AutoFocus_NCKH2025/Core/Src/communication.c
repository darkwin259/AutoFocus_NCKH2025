/*
 * communication.c
 *
 *  Created on: Mar 3, 2026
 *      Author: Ad
 */


#include"communication.h"
extern UART_HandleTypeDef huart1;
uint8_t rx_data[8];
uint8_t cmd_ready = 0;

/*
 *
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if(huart->Instance == USART1) {
//        // Kiểm tra Header 0xAA và Tail 0x55
//        if(rx_data[0] == 0xAA && rx_data[7] == 0x55) {
//            // Kiểm tra Checksum XOR
//            uint8_t calc_checksum = rx_data[1] ^ rx_data[2] ^ rx_data[3] ^ rx_data[4] ^ rx_data[5];
//            if(calc_checksum == rx_data[6]) {
//                uint8_t cmd = rx_data[2];
//                uint8_t dir = rx_data[3];
//                uint16_t steps = (rx_data[4] << 8) | rx_data[5];
//
//                if(cmd == 0x01) { // Lệnh di chuyển Focus
//                    MS41929_SetDirection(dir);
//                    MS41929_Step(steps);
//
//                    // Gửi lại ACK cho Pi báo đã xong
//                    uint8_t ack[] = {0xAA, 0x01, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x55};
//                    HAL_UART_Transmit(&huart1, ack, 8, 10);
//                }
//            }
//        }

    	if(Parse_Check_Error(rx_data)){
    		uint8_t ack[] = {0xAA, 0x01, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x55};
    	     HAL_UART_Transmit(&huart1, ack, 8, 10);
    	}
    	else{
    		 char error_msg[] = "ERROR Frame\r\n";
    		 HAL_UART_Transmit(&huart1, (uint8_t*)error_msg, strlen(error_msg), 100);
    	}
        // Tiếp tục nhận gói tin tiếp theo
        HAL_UART_Receive_DMA(&huart1, rx_data, 8);
    }
}
/*
 * @brief: Kiểm tra khung dữ liệu trả về và báo lối cho CPU
 * @param: Dữ liệu 8bit từ Máy tính nhúng
 * @retval: 1: Sure , 0: Error package
 * @example: Khung dữ liệu sẽ dạng  Header:0xAA|Id|CMD|DIR|DATAHIGH|DATALOW|CHECKSUM|Tail:0x55

 */
void Parse_Check_Error(uint8_t data[]){
	// 1. Kiểm tra Header (Byte 0)
	    if (data[0] != 0xAA) {
	        // Sai Header:
	        return 0;
	    }

	    // 2. Kiểm tra Tail (Byte 7)
	    if (data[7] != 0x55) {
	        // Sai Tail:
	        return 0;
	    }
	    // 3. Kiểm tra xem có phải thiết bị 1 không
	    if (data[1] != 0x01) {
	           //Chỉ sử dụng 1 thiết bị
	           return 0;
	       }
	    // 4. Kiểm tra xem tính toàn vẹn của dữ liệu
	    	// sử dụng hàm check sum XOR từ bit 1 đến bit 5, nếu khác bit6 là sai dữ liệu
	    uint8_t caculated_checksum =data[1]^data[2]^data[3]^data[4]^data[5];
	    if(caculated_checksum != data[6])
	    	return 0;
      //.........................................Final check Frame..................................................
	  //.........................................Parsing Data.......................................................
	    uint8_t command=data[2];
	    uint8_t direction = data[3];
	    uint16_t steps = (uint16_t)((data[4] << 8) | data[5]);

	    switch(command){
				case 0x01:
					//Move focus
					MS41929_SetDirection(direction);
					MS41929_Step(steps);
					break;
				case 0x02:
					//IR-Cut setting
					break;
				case 0x03:
					//Chạy về vị trí kịch kim
					break;
				case 0x04:
					//Reset MS41929
					HAL_GPIO_WritePin(MS_RSTB_GPIO_Port, MS_RSTB_Pin, GPIO_PIN_RESET);
					HAL_Delay(10);
					HAL_GPIO_WritePin(MS_RSTB_GPIO_Port, MS_RSTB_Pin, GPIO_PIN_SET);
					HAL_Delay(10);
					break;
				default:
					return 0;
	    }

					return 1;
}
