&ensp; &ensp; 该系统原为无人驾驶系统中的辅助系统——远程驾驶,核心功能:车端推送视频,驾驶端拉取视频,实时查看车端行驶情况,并可以远程驾驶车端行驶(需要罗技方向盘)。  
&ensp; &ensp; 虽然名为远程驾驶系统,但其实核心技术是跨网络下的视频推拉流,只是其中附加了车端远程操控功能。如果只是简单的实现流媒体功能,则不需要罗技方向盘等车载硬件设施,可通过简单的部署,实现局域网和广域网下的视频推拉流。
# 模块说明  
主要分为4个模块:驾驶端、车端、调度服务和RTSP服务  
* 驾驶端(app/driver):驾驶端主程序,拉取车端视频  
* 车端(app/car):车端主程序,连接调度服务,完成车端视频推流,最终响应驾驶端对车端的控制          
* 车端桥接程序(app/car_bridge):用于车端主程序和ros系统之间数据的中转(不用python和c++的原因:go不需要安装ros环境,就可以编译通过)
* 调度服务(go_server/dispatch):主要的服务端程序,中转车端与驾驶端数据        
* RTSP服务(third_party/EasyDarwin-linux-8.1.0-1901141151):RTSP流媒体服务器   
* 后勤服务(go_server/logistics):负责客户端、图片等资源的下载，后期可以实现某些信息的查询(比如所有服务的地址，以取消客户端的配置)  
注:模块名后括号中为路径    
# 目录说明  
app:所有客户端程序   
doc:公共帮助文档  
go_server:go服务  
third_party:第三方库  
# 编译、打包和部署
[驾驶端](./app/README.md) | [车端](./app/README.md) | [车端桥接程序](./app/README.md) | [调度服务](./go_server/README.md) |  [后勤服务](./go_server/README.md)
# 远程驾驶系统部署  
* 后端服务部署
```
1.服务器上运行调度服务dispatch    
2.服务器上运行后勤服务logistics 
3.服务器上运行RTSP服务(通过EasyDarwin目录中start.sh脚本，启动RTSP服务)
```  
注:如果只是使用流媒体功能,可以取消logistics服务的部署
* 车端部署(Linux)
```
1.车端宿主机上运行car_bridge   
2.车端docker中启动车端主程序car
3.设置车端主程序car配置文件ConfigCar.ini(参看该文档中注释)  
```
注:车端主程序详细部署查看[车端主程序部署](./app/README.md)。如果车端Linux系统可以直接运行Qt6程序,则可以直接本地系统(宿主机)运行车端主程序,不使用docker容器中运行车端主程序。如果只是简单的使用流媒体功能,可以取消部署car_bridge
* 驾驶端部署(Windows)
```
1.通过驾驶端安装包安装远程驾驶2.0
```
注：需要提前安装[罗技方向盘驱动](https://download01.logi.com/web/ftp/pub/techsupport/gaming/lghub_installer.exe)