# Enable WebUSB to allow FDIM controller configuration under Windows

By default Windows locks USB device port and do not allow Chrome to interact with devices from WebUSB-enabled web pages.
In order to switch USB port to proper mode, one needs to download Zadig software and switch USB device driver to WinUSB

1. Download Zadig from [https://zadig.akeo.ie/](https://zadig.akeo.ie/) and install it.

2. Go to Options and enable List All Devices.

![](zadig1.png)

3. Select Interface 0 of Arduino Pro Micro from the list.

![](zadig2.png)

4. Select WinUSB driver to the right. Driver on the left side may be any. Click Install Driver button.

![](zadig3.png)

5. Driver installation will finish with confirmation dialog window. Close it.

![](zadig4.png)

6. Select Interface 2 of Arduino Pro Micro from the list and click Install Driver button again

![](zadig5.png)

7. For both interfaces you should have WinUSB driver listed on the left

![](zadig7.png)

8. You can go back to [Configurator page](https://p1ne.github.io/fdim-controller/fdim-config/). You may need to reload it several times, then click Connect, pair device and start configuring
 
