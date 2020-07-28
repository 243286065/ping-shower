# ping-shower
功能: 可视化显示当前ping的rtt值和丢包率.

运行环境: 目前在Linux上运行成功,理论上在Windows也是可以的,不过尚未适配,可能会有些库找不到.

## 编译和安装
首先请安装 `matplotlib`, 本程序使用它作图:
```
sudo apt-get install python-matplotlib python-numpy python2.7-dev
```

然后使用cmake进行进行:
```
git submodule update --init

cmake .
make
```

## 使用
在Linux上运行需要管理员权限:
```
sudo x86_64/bin/ping_shower 192.168.1.104
```

## 使用效果
![image](https://github.com/243286065/pictures_markdown/blob/master/tools/f75d192460770b41d7ee9aea3ac34804.png?raw=true)