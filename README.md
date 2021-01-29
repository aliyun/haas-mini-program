## HaaS轻应用框架

### HaaS
Hardware as a Service 硬件即服务，提供物联网软硬件的统一开发框架和服务。

### 轻应用框架
提供了支撑[轻应用](https://help.aliyun.com/document_detail/174810.html)运行所需的全套基础设施。  
是一套集成了包括硬件抽象、网络接口、物联网组件、运行时引擎、对象封装等功能的应用软件框架。

**轻应用**之所以**轻**，是因为有如下特点：
- 硬件设备的开发可以不再依赖嵌入式软件技术栈，调用简洁的 [API](https://help.aliyun.com/document_detail/200577.html) 即可操控硬件行为。  
- 采用解释型编程语言JavaScript，整个开发过程无需搭建嵌入式开发环境，也无需编译、烧写。
- 支持一键[热更新](http://help.aliyun.com/document_detail/184021.html)推送应用代码到硬件，极致开发体验。  

### 开发说明
#### 应用开发者
如果您希望开发轻应用，可以移步[HaaS轻应用 - 使用文档](https://help.aliyun.com/document_detail/174810.html)，无需理解具体的实现原理。

#### 系统和框架开发者
如果希望修改框架或移植到您的硬件平台，可以参考下述介绍。

### 目录结构
```
.
├── aos.mk               // 在AliOS Things上的编译脚本
├── Makefile             // 在Linux模拟环境的编译脚本
├── adapter              // 硬件和系统适配层
│   │── include          // 适配层接口定义
│   └── platform         // 不同平台的适配层实现
│       ├── aos-haas100  // 在HaaS100上基于AliOS Things的适配层参考实现
│       └── linux        // 在Linux模拟环境的适配层参考实现
├── components           // 功能组件
├── engine               // 运行时引擎
├── entry                // 框架主入口
├── example              // 示例代码
├── libjs                // 对象封装
├── main                 // 框架主任务
├── services             // 框架内部服务
├── test                 // 测试代码
├── tools                // 编译工具
└── utils                // 依赖的基础库
```
> 注：`aos-haas100`是在[HaaS100](https://help.aliyun.com/document_detail/184426.html)上基于[AliOS Things](https://github.com/alibaba/AliOS-Things)的适配层参考实现。

### 移植说明

#### 移植流程
1. 在 `adapter/platform/` 下新增一个目标硬件平台的目录。
2. 实现 `adapter/include/` 下定义的适配层接口。运行框架核心功能，需要完成以下标注`(关键)`的接口适配。
3. 修改`Makefile`并交叉编译，或参考`Makefile`将源码添加到目标硬件平台的项目环境中完成编译。

#### 适配层说明
```
adapter
├── include
│   ├── amp_fs.h             // 文件系统 (关键)
│   ├── amp_httpc.h          // HTTP客户端
│   ├── amp_kv.h             // 键值对存储 (关键)
│   ├── amp_location.h       // 位置服务
│   ├── amp_network.h        // 网络管理 (关键)
│   ├── amp_ota.h            // OTA系统升级
│   ├── amp_pcm.h            // 音频PCM读写
│   ├── amp_pm.h             // 电源管理
│   ├── amp_socket.h         // Socket (关键)
│   ├── amp_system.h         // 系统 (关键)
│   ├── amp_tcp.h            // TCP (关键)
│   ├── amp_tts.h            // TTS语音
│   ├── amp_udp.h            // UDP (关键)
│   └── peripheral           // 硬件外设
│       ├── amp_hal_adc.h
│       ├── amp_hal_can.h
│       ├── amp_hal_dac.h
│       ├── amp_hal_flash.h
│       ├── amp_hal_gpio.h
│       ├── amp_hal_i2c.h
│       ├── amp_hal_pwm.h
│       ├── amp_hal_rtc.h
│       ├── amp_hal_spi.h
│       ├── amp_hal_timer.h
│       ├── amp_hal_uart.h   // 串口 (关键)
│       └── amp_hal_wdg.h
└── platform
    ├── aos-haas100
    └── linux
```

### 致谢
HaaS轻应用站在巨人的肩上，由关键的开源项目强力驱动：
- [AliOS Things](https://github.com/alibaba/AliOS-Things) - 开源物联网操作系统
- [QuickJS](https://bellard.org/quickjs) - Bellard出品，不解释
- [Duktape](https://github.com/svaarala/duktape) - 资源开销与性能兼备的JavaScript引擎

HaaS轻应用也参考了以下优秀的开源项目：
- [Mongoose-OS](https://github.com/cesanta/mongoose-os) - 采用mJS引擎的IoT固件开发框架
- [Espruino](https://github.com/espruino/Espruino) - 可以在MCU上运行的JavaScript框架
- [Johnny-Five](https://github.com/rwaldron/johnny-five) - 用于搭建机器人和IoT的JavaScript框架
- [Cylon.js](https://github.com/hybridgroup/cylon) - 支持了40+种硬件平台的机器人和IoT编程框架

### License
[Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0)
