import cv2
import numpy as np
from picamera2 import Picamera2
import time

# Khởi tạo camera
picam2 = Picamera2()
picam2.configure(picam2.create_preview_configuration(
    main={"format": "RGB888", "size": (640, 480)}
))
picam2.start()

time.sleep(1)  # chờ camera ổn định

def sobel_sharpness(gray):
    # Sobel theo X và Y
    sobelx = cv2.Sobel(gray, cv2.CV_64F, 1, 0, ksize=3)
    sobely = cv2.Sobel(gray, cv2.CV_64F, 0, 1, ksize=3)

    magnitude = np.abs(sobelx) + np.abs(sobely)
    sharpness = np.mean(magnitude)

    return sharpness

while True:
    
    frame = picam2.capture_array()
    gray = cv2.cvtColor(frame, cv2.COLOR_RGB2GRAY)
    sharpness = sobel_sharpness(gray)

    # Hiển thị
    cv2.putText(frame, f"DO NET: {sharpness:.2f}",
                (10, 30), cv2.FONT_HERSHEY_SIMPLEX,
                0.8, (0, 255, 0), 2)

    cv2.imshow("Camera", frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cv2.destroyAllWindows()
picam2.stop()
