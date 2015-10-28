本程序适用于MAC和Linux下，本程序默认通过读取rom.conf和ads.conf文件来配置
===============
目录:

-ads    # 放置广告包, 如ads/gg, ads/nc

-base   # 放置机型底包, 如base/zte/n986, base/xiaomi/2a

-over   # 放在里面的文件都会copy到system里面, 一般放sb

文件:

-rom.conf # 内容: zte\_n986 = true (代表base/zte/n986)

-ads.conf # 内容: gg = true (代表ads/gg)

-ads/gg.conf # 内容: app.package\_app.class\_app标题
