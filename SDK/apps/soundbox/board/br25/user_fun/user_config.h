#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_

#define USER_DEV_USB	"udisk0"
#define USER_DEV_SD0	"sd0"
#define USER_DEV_SD1	"sd1"

#define USER_PA_MODE_1 1//双io功放
#define USER_PA_MODE_2 2//单io功放 电压
#define USER_PA_MODE_3 3//单io功放 脉冲
#define USER_PA_MODE_AUTO 3//自动检测

#define USER_PA_MUTE_H    1//高mute
#define USER_PA_MUTE_L    0//低mute

#define USER_PA_AB_H    1//高ab类
#define USER_PA_AB_L    0//低ab类

#define USER_RGB_LOOP_MODE_1    1//所有
#define USER_RGB_LOOP_MODE_2    2//非单色闪烁循环
#define USER_RGB_LOOP_MODE_3    3//单色闪烁循环

#define USER_PA_EN  1//功放总开关
#define USER_PA_CLASS USER_PA_MODE_2//功放类型

#define USER_SDK_BUG_1  0//使用tone_play_with_callback_by_name该接口播放提示音 有概率出现没有声音出来 之后也没有声音。主要是解码没唤醒导致
#define USER_SDK_BUG_2  1//linein 模式下 采用dac L进 dac R出的方式，进linein之后声音小
#define USER_SDK_BUG_5  1//bt、FM模式提示音为MP3异常

#define USER_BT_VBAT_DISPLAY    0//手机电量显示 但不要通话功能  需要打开电量显示宏 BT_SUPPORT_DISPLAY_BAT 与通话协议宏 USER_SUPPORT_PROFILE_HFP
#define USER_FM_MODE_SYS_VOL    20//进FM 大于此音量设置为此音量 退出时恢复

#define USER_FIRST_BOOT_VOL     (SYS_MAX_VOL*2/3)//first boot 第一次上电开机音量
#define USER_POWER_ON_VOL       0//USER_FIRST_BOOT_VOL//每次开机音量 0或者注释 为系统默认
#define USER_TONE_PLAY_MODE     1//1:打断方式播放提示音 0:嵌入方式播放提示音
#define USER_MUSIC_TO_BT        1//music 下只有一个设备时 拔出设备 跳到bt模式
#define USER_USB_OR_SD	        USER_DEV_USB//进music模式优先设备
#define USER_POWER_ON_INIT      1//等开机提示音播放完之后再切到上线设备
#define USER_IR_PLAY_FILE_NUMBER      1//遙控器播放文件
#define USER_IR_TWS_KEY_FILTER_EN    1//TWS 过滤重复iryey消息
#define USER_IR_TWS_SYNC_DEL_INFO_EN    1//TWS 同时删除对箱信息并断开对箱
#define USER_EQ_FILE_ADD_EQ_TABLE     1//使用外部配置eq的同时还使用软件中的eq效果  注意：可能影响到外部配置通话eq 未与珠海确认
#define USER_EQ_BASS_INDEX      1//BASS 调节第几个索引 USER_EQ_BASS_INDEX不要超出eq 段
#define USER_EQ_TERBLE_INDEX    8//TERBLE 调节第几个索引 
#define USER_IR_POWER           1//遥控器假关机 
#define USER_BT_TONE_PLAY_GO_INIT  1//BT 播放完提示音再初始化蓝牙模式 避免bt提示音未播放完 被回连提示音打断
#define USER_MIC_DEFAULT_GAIN       10//default mic 默认音量
#define USER_RECORD_EN          1//录音使能

#define USER_POWERON_MEMORY_MUSIC_VOL      1//保存music vol
#define USER_ADKEY_MAPPING_EN   1//AD KEY 按鍵值映射
#define USER_TONE_PLAY_ERROR_NO_RETURN  1//使用有回调函数的接口播放 提示音 提示音播放错误 不需要执行回调函数

#define USER_MIC_MUSIC_VOL_SEPARATE  0//music mic 音量分开调节
#define USER_POWER_LOW_DOW_VOL_EN   1//低电降音量 0：不开启 

#define USER_TWS_ADD_DELL_TWS_INFO 1//手机连接不能影响tws连接与断开
#define USER_WAKEUP_EN          0//wakeup_param 软关机是否是能唤醒

#if ((EQ_SECTION_MAX<=USER_EQ_BASS_INDEX || EQ_SECTION_MAX<=USER_EQ_TERBLE_INDEX))
#error "高低音超出总eq段"
#endif
#if (USER_EQ_TERBLE_INDEX ==USER_EQ_BASS_INDEX)
#error "高低音调节段位不能相同"
#endif

#endif