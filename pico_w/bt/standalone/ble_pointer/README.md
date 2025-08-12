### BLE pointer

This example is based on BTstack's 'hog_mouse_demo' and demonstrates a Bluetooth HID mouse. Cursor position is controlled by mpu6050 angle measurements, allowing you to point at the screen and move the mouse cursor. 

To use this example connect a mpu6050 (which can be found at https://thepihut.com/products/6-dof-sensor-mpu6050) to the pico, with SDA connected to pin 4 and SCL connected to pin 5. Also, connect 2 buttons to pins 15 and 16 for left and right click.

Once powered, you should be able to pair your computer with 'HID Mouse' and use the pointer. 
