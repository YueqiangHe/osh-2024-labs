# lab1_report  
何跃强 PB22111649  
## phase 1 : Linux编译  
裁剪内核分为五个探究步骤  
**第一步**：去除无关的 device 设置  
**第二步**：去除Network 相关  
**第三步**：去除security 相关  
**第四步**：去除kernal kacking 相关
**第五步**：将General Setup 里面的 Gzip -> LAMA , -O2 -> -O
最后裁剪得到：  
![da2e8903516d8a1b5005483e835adcc](https://github.com/YueqiangHe/osh-2024-labs/assets/144820167/7f3aa356-7a8f-41e9-9636-768080b14503)  
并且可以编译（但由于后来重新编译了，bzimage丢失，最后交phase3的bzimage） 
我们发现在**第二步，第四步，第五步**让内核减少的最多。  
