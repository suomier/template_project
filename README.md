# 项目模板

主要是将一些常见的组件集成到项目中，在需要使用时，能够快速搭建项目



项目模板规划

```

template_base    基础模板
template_ffmpeg  ffmpeg项目模板
template_sdk     sdk项目模板

# 服务器项目模板   
svr_template_libevent        libevent 封装的服务器项目模板
svr_template_libuv           libuv 封装的服务器项目模板          
svr_template_state-threads   state-threads 封装的服务器项目模板
svr_template_coroutines      C++协程封装的服务器项目模板
```



# template_base

文件结构

```
├─common    template_common: 引入外部库; 内部处理: 日志, 配置, 外部接口交互, 跨平台等;
├─conf      配置文件
├─example   例子: 方便快速验证语法; cmake配置时, 会将其中每个文件配置对应的exe; 
└─test      测试: gtest单元测试目录; 
```







