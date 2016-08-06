Windows 10 version 1607 (OS Build 14393.10) a.k.a. Anniversary update removed WebCam MJPG from supported media types replaceing them with NV12 for MMF (Microsoft Media Foundation) and dropping MJPG completely for DirectShow. KsStudio and USBView still show that all three webcams have UVC MJPG descriptors intact and available at Windows Kernel Streaming.

<pre>
TEST RESULTS:

Windows 10 version 1511 (OS Build 10586.494)

MMF camera 0 Logitech HD Pro Webcam C920 \\?\usb#vid_046d&pid_082d&mi_00#6&17c8b6a4&0&0000#{e5323777-f976-4f5b-9b55-b94699c46e44}\{bbefb6c7-2fc4-4139-bb8b-a58bba724083}
[ 0] MJPG   640x480  @ 30
...
[16] MJPG  1920x1080 @ 30
[17] RGB24  640x480  @ 30
...
[35] RGB24 2304x1536 @  2
[36] I420   640x480  @ 30
...
[54] I420  2304x1536 @  2

DirectShow camera 0 Logitech HD Pro Webcam C920 \\?\usb#vid_046d&pid_082d&mi_00#6&17c8b6a4&0&0000#{65e8773d-8f56-11d0-a3b9-00a0c9223196}\{bbefb6c7-2fc4-4139-bb8b-a58bba724083}
[ 0] MJPG   640 x 480
...
[16] MJPG  1920 x 1080
...
[17] RGB24  640 x 480
...
[35] RGB24 2304 x 1536
[36] I420   640 x 480
...
[54] I420  2304 x 1536

Windows 10 version 1607 (OS Build 14393.10)

MMF camera 0 Logitech HD Pro Webcam C920 \\?\usb#vid_046d&pid_082d&mi_00#6&17c8b6a4&0&0000#{e5323777-f976-4f5b-9b55-b94699c46e44}\{bbefb6c7-2fc4-4139-bb8b-a58bba724083}
[ 0] NV12   640x480  @ 30
...
[16] NV12  1920x1080 @ 30
[17] RGB24  640x480  @ 30
...
[35] RGB24 2304x1536 @  2
[36] I420   640x480  @ 30
...
[54] I420  2304x1536 @  2

DirectShow camera 0 Logitech HD Pro Webcam C920 \\?\usb#vid_046d&pid_082d&mi_00#6&17c8b6a4&0&0000#{65e8773d-8f56-11d0-a3b9-00a0c9223196}\{bbefb6c7-2fc4-4139-bb8b-a58bba724083}
[ 0] YUY2   640 x 480
...
[16] YUY2  1920 x 1080
[17] RGB24  640 x 480
...
[35] RGB24 2304 x 1536
[36] I420   640 x 480
...
[54] I420  2304 x 1536

Same results with:
FaceTime HD Camera (Built-in) \\?\usb#vid_05ac&pid_8509&mi_00#7&1c384fb4&0&0000#{65e8773d-8f56-11d0-a3b9-00a0c9223196}\global
Realtek RTS5880 \\?\usb#vid_25be&pid_0003&mi_00#6&ab46da2&1&0000#{65e8773d-8f56-11d0-a3b9-00a0c9223196}\global
(no access to MJPEG, no replacement)
</pre>
