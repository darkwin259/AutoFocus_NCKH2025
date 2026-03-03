# 📷 AutoFocus NCKH2025 - Smart Lens Control System

Hệ thống tự động lấy nét (Auto Focus) kết hợp Computer Vision, Trí tuệ nhân tạo (AI) và vi điều khiển thời gian thực STM32.

## 🏗 Kiến trúc hệ thống
1. **The Brain (Software):** Jetson Nano / Raspberry Pi 4B chạy AI (Object Detection) và thuật toán Computer Vision (Variance of Laplacian) để tìm điểm nét tối ưu (Hill Climbing).
2. **The Muscle (Firmware):** STM32F411 điều khiển IC Motor MS41929 qua SPI/PWM với độ chính xác 256 microsteps. Giao tiếp qua RS485.

## 📂 Cấu trúc thư mục
- `/Firmware`: Mã nguồn C/C++ cho STM32 (HAL Library, MS41929 Driver).
- `/Software`: Mã nguồn Python (OpenCV, AI Model, Serial Protocol).
- `/Hardware`: Sơ đồ nguyên lý (Schematic) & PCB Layout.
- `/Docs`: Tài liệu báo cáo, Slide, Datasheet.

## ⚙️ Cài đặt (Software)
```bash
cd Software
pip install -r requirements.txt