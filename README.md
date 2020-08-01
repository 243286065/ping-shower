# ping-shower
功能: 可视化显示当前ping的rtt值和丢包率.

运行环境: 目前在Linux上运行成功,理论上在Windows也是可以的,不过尚未适配,可能会有些库找不到.

## 安装
首先请安装 `matplotlib`, 本程序使用它作图:
```
pip install matplotlib
```

由于改用python实现,因此无需编译直接可以运行.

## 使用
```
python ping.py ip
```
> 注意Linux上需要使用root权限运行

## 使用效果
![image](https://github.com/243286065/pictures_markdown/blob/master/tools/f75d192460770b41d7ee9aea3ac34804.png?raw=true)