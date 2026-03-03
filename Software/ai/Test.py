import cv2
import numpy as np
import time
import os
import sys

# Đảm bảo terminal hiển thị được tiếng Việt (UTF-8)
if sys.stdout.encoding != 'utf-8':
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except AttributeError:
        pass

# --- CẤU HÌNH ---
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
VIDEO_PATH = os.path.join(BASE_DIR, "sample.mp4") 
ROI_SIZE = 200                
THRESHOLD_STEP = 5            

class AdaptiveFocusMetric:
    def __init__(self):
        # Trọng số khởi tạo: a (Sobel), b (Tenengrad), c (Laplacian)
        self.a, self.b, self.c = 0.4, 0.3, 0.3

    def calculate_metrics(self, roi):
        """Tính toán đồng thời 3 độ đo độ nét"""
        # Tiền xử lý: Lọc nhiễu Gaussian nhẹ để làm mịn hạt nhiễu (Noise)
        blurred = cv2.GaussianBlur(roi, (3, 3), 0)

        # 1. Sobel (Đạo hàm bậc 1) - Nhạy khi ảnh rất mờ
        sobelx = cv2.Sobel(blurred, cv2.CV_64F, 1, 0, ksize=3)
        sobely = cv2.Sobel(blurred, cv2.CV_64F, 0, 1, ksize=3)
        f_sobel = np.mean(np.abs(sobelx) + np.abs(sobely))

        # 2. Tenengrad (Bình phương Sobel) - Nhấn mạnh các cạnh rõ
        f_tenengrad = np.mean(np.square(sobelx) + np.square(sobely))

        # 3. Laplacian Variance (Đạo hàm bậc 2) - Cực nhạy khi gần điểm nét
        f_laplacian = cv2.Laplacian(blurred, cv2.CV_64F).var()

        return f_sobel, f_tenengrad, f_laplacian

    def get_fusion_score(self, roi):
        s, t, l = self.calculate_metrics(roi)

        # LOGIC THÍCH NGHI (Adaptive Logic)
        # Nếu ảnh rất mờ (Laplacian thấp), tin tưởng Sobel để tìm hướng di chuyển nhanh
        if l < 50:
            self.a, self.b, self.c = 0.7, 0.2, 0.1
        # Nếu ảnh đã khá nét (Laplacian cao), tin tưởng Laplacian để tinh chỉnh micro-step
        elif l > 200:
            self.a, self.b, self.c = 0.1, 0.2, 0.7
        else:
            self.a, self.b, self.c = 0.4, 0.3, 0.3

        # Chuẩn hóa quy mô (Scaling): Tenengrad và Laplacian thường có giá trị rất lớn
        # Chúng ta dùng căn bậc 2 để đưa chúng về cùng quy mô với Sobel trước khi nhân trọng số
        combined_score = (self.a * s) + (self.b * np.sqrt(t)) + (self.c * np.sqrt(l))
        
        return combined_score, (s, t, l), (self.a, self.b, self.c)

def main():
    cap = cv2.VideoCapture(VIDEO_PATH)
    if not cap.isOpened():
        print(f"LỖI: Không mở được file video tại {VIDEO_PATH}!")
        return

    print("--- BẮT ĐẦU MÔ PHỎNG LẤY NÉT THÍCH NGHI (ADAPTIVE FUSION) ---")
    af_metric = AdaptiveFocusMetric()
    last_score = 0

    while cap.isOpened():
        ret, frame = cap.read()
        if not ret: break

        # 1. Tiền xử lý
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        h, w = gray.shape
        
        # 2. Cắt vùng ROI trung tâm
        start_x = w // 2 - ROI_SIZE // 2
        start_y = h // 2 - ROI_SIZE // 2
        roi = gray[start_y : start_y + ROI_SIZE, start_x : start_x + ROI_SIZE]
        
        # 3. Tính điểm độ nét theo hệ thống Adaptive Fusion
        current_score, raw_metrics, weights = af_metric.get_fusion_score(roi)
        s, t, l = raw_metrics
        wa, wb, wc = weights
        
        # 4. Giả lập Logic điều khiển Motor
        diff = current_score - last_score
        cmd_msg = "STABLE"
        if abs(diff) > 1.0: # Ngưỡng nhạy cho điểm fusion
            cmd_msg = "FOCUS IMPROVING" if diff > 0 else "FOCUS DROPPING"
        last_score = current_score

        # 5. Hiển thị giao diện NCKH chuyên nghiệp
        cv2.rectangle(frame, (start_x, start_y), (start_x + ROI_SIZE, start_y + ROI_SIZE), (255, 0, 0), 2)
        
        # Overlay thông số metrics
        bg_color = (0, 0, 0)
        cv2.putText(frame, f"FUSION SCORE: {current_score:.2f}", (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
        cv2.putText(frame, f"Weights: a={wa:.1f} b={wb:.1f} c={wc:.1f}", (20, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 1)
        
        # Hiển thị dữ liệu thô (Raw Data) phục vụ báo cáo
        cv2.putText(frame, f"S:{s:.1f} T:{t:.1f} L:{l:.1f}", (20, 100), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)
        cv2.putText(frame, f"CMD: {cmd_msg}", (20, 130), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0) if diff > 0 else (0, 0, 255), 2)

        cv2.imshow("NCKH 2025 - Adaptive Multi-Metric Focus", frame)

        if cv2.waitKey(25) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()