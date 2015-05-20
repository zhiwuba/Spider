# Spider

高性能爬虫引擎，

已用于GIF库中,从微博/主流网站抓取GIF图片和点评,目前每小时下载1万条数据和GIF图片(受带宽影响)。

包含线程池、网页去重、历史记录、网页分析、epoll/select异步请求管理、Cookie管理、通用Http请求、异步DNS解析等模块。


#Build
依赖库:
boost_1_57_0 提供智能指针
crypto_5_60  提供加密
libevent-2.0.22-stable 提供异步DNS解析
mpir-2.7.0   提供大数的处理
