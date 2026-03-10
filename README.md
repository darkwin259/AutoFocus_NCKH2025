# 📷 AutoFocus NCKH2025 - Smart Lens Control System

**Hệ thống tự động lấy nét (Auto Focus) cho cụm camera quan sát trên thiết bị bay UAV/Robot tự hành.**

Dự án này kết hợp các công nghệ Computer Vision, Trí tuệ nhân tạo (AI/ML) và Vi điều khiển thời gian thực STM32 nhằm giải quyết bài toán lấy nét tự động trong môi trường rung nhiễu mạnh và biến động ánh sáng - vốn rất phổ biến trong các ứng dụng quang điện tử quân sự và trinh sát.

## 🌟 Chức Năng Cốt Lõi (Core Features)
- **Real-time Image Processing:** Tích hợp kỹ thuật phân tích ROI (Region of Interest) với độ trễ thấp dựa trên Python (Threading/Multiprocessing).
- **Adaptive Multi-metric Fusion:** Đánh giá độ nét (Focus Metric) bằng cách kết hợp linh hoạt các toán tử **Laplacian**, **Tenengrad**, **Sobel** và **Variance**.
- **Tối ưu hóa điểm lấy nét:** Ứng dụng các thuật toán tìm kiếm vị trí quang học tối ưu như **Golden Section Search** và **Hill Climbing** giúp hội tụ nhanh.
- **Robotics Control (Real-time Firmware):** Vi điều khiển STM32F411 (Cortex-M4) điều khiển IC Motor MS41929 giao tiếp qua SPI và xuất xung PWM điều khiển Stepper Motor (độ phân giải cao lên đến 256 microsteps).
- **Giao thức truyền tin tin cậy (Protocol):** Giao tiếp UART/RS485 giữa bộ xử lý cao cấp (Pi 4/Jetson) và vi điều khiển (STM32) được bảo vệ bằng cơ chế Checksum (**CRC8/CRC16**).

## 🏗 Kiến Trúc Hệ Thống (System Architecture)

Hệ thống được thiết kế theo cấu trúc Master-Slave đồng bộ:
1. **The Brain (Software/Master):** 
   - **Nền tảng:** Phù hợp với Raspberry Pi 4B / Jetson Nano
   - **Môi trường:** Python 3.x, OpenCV
   - **Nhiệm vụ:** Xử lý khối lượng tính toán lớn (Object Detection, đánh giá hàm độ sắc nét của hệ thống quang học), chạy thuật toán lấy nét để tìm ra điểm tối ưu và gửi lệnh dịch chuyển xuôi/ngược tới hệ thống cơ học.
   
2. **The Muscle (Firmware/Slave):** 
   - **Vi điều khiển:** STM32F411CEU6 (Thư viện HAL, lập trình bằng C/C++)
   - **Nhiệm vụ:** Đọc lệnh điều khiển qua mạng RS485 (ứng dụng kỹ thuật ngắt DMA), điều khiển trực tiếp động cơ bước có bao quát gia tốc (Ramp-up/Ramp-down) và kiểm soát an toàn vật lý qua công tắc hành trình (Limit Switches).

## 📂 Tổ Chức Thư Mục (Repository Structure)

```plaintext
AutoFocus_NCKH2025/
├── Firmware/
│   └── AutoFocus_NCKH2025/    # Mã C/C++ cho STM32 (Driver MS41929, DMA, UART, SPI, Timer PWM)
├── Software/
│   ├── ai/                    # Module model Deep Learning hỗ trợ nhận diện và tracking (CNN)
│   ├── comm/                  # Protocol truyền/nhận lệnh UART/RS485, Checksum validation
│   ├── control/               # Thuật toán sinh quỹ đạo lấy nét (Hill Climbing, Golden Section)
│   ├── vision/                # Module xử lý ảnh: Laplacian, Sobel, Tenengrad, ROI
│   ├── main.py                # Flow chính của tầng "The Brain"  
│   └── requirements.txt       # Dependencies cho môi trường Python
├── Hardware/                  # Sơ đồ nguyên lý (Schematics) và PCB Layout
└── Docs/                      # Báo cáo NCKH, Slide trình bày, Datasheets của MS41929, v.v.
```

## ⚙️ Cài Đặt Khởi Tạo (Getting Started)

### 1. Khởi chạy Software (Node xử lý AI/CV)
Bạn cần Python 3.8+ và đã cài đặt các công cụ biên dịch thiết yếu của OpenCV.
```bash
cd Software
pip install -r requirements.txt
python main.py
```

### 2. Biên dịch Firmware (Node điều khiển Motor)
- Mở project tại đường dẫn `Firmware/AutoFocus_NCKH2025` bằng **STM32CubeIDE** hoặc hệ thống Make / Keil MDK.
- Sử dụng Mạch Nạp ST-Link hoặc J-Link.
- Biên dịch (Build) và nạp (Flash/Debug) code điều khiển xuống STM32F411. Có thể sử dụng VS Code với file cấu hình `launch.json` cho GDB OpenOCD/ST-Link.

## 💎 Nguyên Tắc Vận Hành & Lập Trình (Engineering Principles)
1. **Hiệu suất thời gian thực:** Code STM32 cần giải phóng CPU bằng cách triệt để sử dụng ngắt (Interrupt) và DMA cho các thao tác IO (UART/SPI/ADC). Tuyệt đối tránh delay chặn (blocking).
2. **An toàn phần cứng:** Bắt buộc áp dụng Ramp-up/Ramp-down profile cho Motor tránh trượt bước. Mọi tín hiệu từ Limit Switches sẽ được ưu tiên ngắt để chống kẹt cơ học.
3. **Chuẩn mực Firmware:** Mã C tuân thủ kiến trúc lớp Hardware -> Driver -> App Logic. Cấu hình Timer, Prescaler cần được comment rõ ràng về mức xung phát ra hợp lý.
4. **Đồng bộ khung truyền:** Mọi gói tin Python <-> STM32 đều có Header, Payload, Checksum (CRC) và kết thúc bằng Terminator/Footer nhằm đảm bảo tính toàn vẹn dư liệu tuyệt đối trên đường RS485 dễ bị nhiễu.

---
*Dự án NCKH: "Nghiên cứu xây dựng hệ tự động lấy nét cho camera trên hệ quang điện tử" (Kỷ yếu NCKH 2025)*