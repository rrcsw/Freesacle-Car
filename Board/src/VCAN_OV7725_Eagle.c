/*!
 *     COPYRIGHT NOTICE
 *     Copyright (c) 2013,山外科技
 *     All rights reserved.
 *     技术讨论：山外论坛 http://www.vcan123.com
 *
 *     除注明出处外，以下所有内容版权均属山外科技所有，未经允许，不得用于商业用途，
 *     修改内容时必须保留山外科技的版权声明。
 *
 * @file       VCAN_OV7725_Eagle.c
 * @brief      鹰眼ov7725驱动代码
 * @author     山外科技
 * @version    v5.0
 * @date       2013-09-07
 */

#include "common.h"
#include "MK60_gpio.h"
#include "MK60_port.h"
#include "MK60_dma.h"
#include "VCAN_camera.h"
#include "VCAN_UI_VAR.h"
#include "Main.h"

//在解压的时候，如果取值为1，则取相应的行数，不然舍去
/*uint8   line_distance[120]={
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1 ,1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};//120//90    */
const uint8   line_distance[120]={         //如果最远处为第1行，则现在解压的是第第11~80，是上边的图像
   0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};//120//90
const uint8   line_distance1[120]={        //这个解压的是下面的行数，完全不合理，实践证明，前瞻过小
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};//120//90


//uint16 camera_boundary=1;//摄uint8     //正中间 16
uint16 camera_boundary=16;//摄uint8      //这里是自己修改过的，表示128列从哪一列开始取


#define OV7725_EAGLE_Delay_ms(time)  DELAY_MS(time)


uint8   *ov7725_eagle_img_buff;

volatile IMG_STATUS_e      ov7725_eagle_img_flag = IMG_FINISH;   //图像状态

//内部函数声明
static uint8 ov7725_eagle_reg_init(void);
static void ov7725_eagle_port_init();


/*!
 *  @brief      鹰眼ov7725初始化
 *  @since      v5.0
 */
uint8 ov7725_eagle_init(uint8 *imgaddr)
{
    ov7725_eagle_img_buff = imgaddr;
    while(ov7725_eagle_reg_init() == 0);
    ov7725_eagle_port_init();
    return 0;
}

/*!
 *  @brief      鹰眼ov7725管脚初始化（内部调用）
 *  @since      v5.0
 */
void ov7725_eagle_port_init()
{
    //DMA通道0初始化，PTA27触发源(默认上升沿)，源地址为PTB_B0_IN，目的地址为：IMG_BUFF，每次传输1Byte
    dma_portx2buff_init(CAMERA_DMA_CH, (void *)&PTB_B0_IN, (void *)ov7725_eagle_img_buff, PTA27, DMA_BYTE1, CAMERA_DMA_NUM, DADDR_KEEPON);

    DMA_DIS(CAMERA_DMA_CH);
    disable_irq(PORTA_IRQn);                        //关闭PTA的中断
    DMA_IRQ_CLEAN(CAMERA_DMA_CH);                   //清除通道传输中断标志位
    DMA_IRQ_EN(CAMERA_DMA_CH);

    port_init(PTA27, ALT1 | DMA_FALLING | PULLDOWN );         //PCLK

    port_init(PTA29, ALT1 | IRQ_RISING  | PULLDOWN | PF);     //场中断，上拉，上降沿触发中断，带滤波

}

/*!
 *  @brief      鹰眼ov7725场中断服务函数
 *  @since      v5.0
 */
void ov7725_eagle_vsync(void)
{

    //场中断需要判断是场结束还是场开始
    if(ov7725_eagle_img_flag == IMG_START)                   //需要开始采集图像
    {
        ov7725_eagle_img_flag = IMG_GATHER;                  //标记图像采集中
        disable_irq(PORTA_IRQn);

#if 1

        PORTA_ISFR = 1 <<  PT27;            //清空PCLK标志位

        DMA_EN(CAMERA_DMA_CH);                  //使能通道CHn 硬件请求
        PORTA_ISFR = 1 <<  PT27;            //清空PCLK标志位
        DMA_DADDR(CAMERA_DMA_CH) = (uint32)ov7725_eagle_img_buff;    //恢复地址

#else
        PORTA_ISFR = 1 <<  PT27;            //清空PCLK标志位
        dma_repeat(CAMERA_DMA_CH, (void *)&PTB_B0_IN, (void *)ov7725_eagle_img_buff,CAMERA_DMA_NUM);
#endif
    }
    else                                        //图像采集错误
    {
        disable_irq(PORTA_IRQn);                        //关闭PTA的中断
        ov7725_eagle_img_flag = IMG_FAIL;                    //标记图像采集失败
    }
}

/*!
 *  @brief      鹰眼ov7725 DMA中断服务函数
 *  @since      v5.0
 */
void ov7725_eagle_dma()
{
    ov7725_eagle_img_flag = IMG_FINISH ;
	camera_zhenshu++;
    DMA_IRQ_CLEAN(CAMERA_DMA_CH);           //清除通道传输中断标志位
}

/*!
 *  @brief      鹰眼ov7725采集图像（采集到的数据存储在 初始化时配置的地址上）
 *  @since      v5.0
 */
void ov7725_eagle_get_img()
{
    ov7725_eagle_img_flag = IMG_START;                   //开始采集图像
    PORTA_ISFR = ~0;                        //写1清中断标志位(必须的，不然回导致一开中断就马上触发中断)
    enable_irq(PORTA_IRQn);                         //允许PTA的中断
    while(ov7725_eagle_img_flag != IMG_FINISH)           //等待图像采集完毕
    {
        if(ov7725_eagle_img_flag == IMG_FAIL)            //假如图像采集错误，则重新开始采集
        {
            ov7725_eagle_img_flag = IMG_START;           //开始采集图像
            PORTA_ISFR = ~0;                //写1清中断标志位(必须的，不然回导致一开中断就马上触发中断)
            enable_irq(PORTA_IRQn);                 //允许PTA的中断
        }
    }
}


//压缩二值化图像解压（空间 换 时间 解压）
//srclen 是二值化图像的占用空间大小
// *dst==>img 19200=160*120  *src==>imgbuff 2400=160*120/8
void img_extract(uint8 *dst, uint8 *src)
{
    uint8 colour[2] = {255, 0}; //0 和 1 分别对应的颜色
    //注：野火的摄像头 0 表示 白色，1表示 黑色
    uint8 tmpsrc;
    static uint8 temp_src[160];
    int i,j;
    static uint8 num_color;
    camera_boundary=flash_camera_boundary;
    for(i=0;i<120;i++)
    {
     num_color=0;
    if(line_distance[119-i]==1)         //从120*160里解压哪些是由自己说了算的，现在只是一个实验而已，留待以后继续摸索
    {                                   //经实践证明，黎锐学长和梁玉亮学长均解压的是上边远处的图像，这样前瞻会大一些，近瞻也不小
      for(j=0;j<20;j++)
      {
       tmpsrc = *src++;
       temp_src[num_color++]=colour[ (tmpsrc >> 7 ) & 0x01 ];
       temp_src[num_color++]=colour[ (tmpsrc >> 6 ) & 0x01 ];
       temp_src[num_color++]=colour[ (tmpsrc >> 5 ) & 0x01 ];
       temp_src[num_color++]=colour[ (tmpsrc >> 4 ) & 0x01 ];
       temp_src[num_color++]=colour[ (tmpsrc >> 3 ) & 0x01 ];
       temp_src[num_color++]=colour[ (tmpsrc >> 2 ) & 0x01 ];
       temp_src[num_color++]=colour[ (tmpsrc >> 1 ) & 0x01 ];
       temp_src[num_color++]=colour[ (tmpsrc >> 0 ) & 0x01 ];
      }

      for(j=camera_boundary;j<camera_boundary+128;j++)   //从120*160里解压哪些是由自己说了算的，现在只是一个实验而已，留待以后继续摸索
      {
       *dst++ = temp_src[j];
//       printf("%d\t",temp_src[j]);       //将解压后的图片用串口发回来
      }
    }
    else
    {
      src+=20;            //src 是未解压过的图片，src+20相当于到下一行
    }                     //不执行if而执行else则表示改行不接压 放弃这一行的意思

    }
}

//extern uint32 flash_yuzhi;
/*OV7725初始化配置表*/
//uint32  eagle_yuzhi;
//eagle_yuzhi=flash_yuzhi;                     //把阈值也放到小液晶里，适应光线
reg_s ov7725_eagle_reg[] =
{
    //寄存器，寄存器值次
    {OV7725_COM4         , 0xC1},       //150帧
    {OV7725_CLKRC        , 0x00},
    {OV7725_COM2         , 0x03},
    {OV7725_COM3         , 0xD0},
    {OV7725_COM7         , 0x40},
    {OV7725_HSTART       , 0x3F},
    {OV7725_HSIZE        , 0x50},
    {OV7725_VSTRT        , 0x03},
    {OV7725_VSIZE        , 0x78},
    {OV7725_HREF         , 0x00},
    {OV7725_SCAL0        , 0x0A},
    {OV7725_AWB_Ctrl0    , 0xE0},
    {OV7725_DSPAuto      , 0xff},
    {OV7725_DSP_Ctrl2    , 0x0C},
    {OV7725_DSP_Ctrl3    , 0x00},
    {OV7725_DSP_Ctrl4    , 0x00},

#if (CAMERA_W == 80)
    {OV7725_HOutSize     , 0x14},
#elif (CAMERA_W == 160)
    {OV7725_HOutSize     , 0x28},
#elif (CAMERA_W == 240)
    {OV7725_HOutSize     , 0x3c},
#elif (CAMERA_W == 320)
    {OV7725_HOutSize     , 0x50},
#else

#endif

#if (CAMERA_H == 60 )
    {OV7725_VOutSize     , 0x1E},
#elif (CAMERA_H == 120 )
    {OV7725_VOutSize     , 0x3c},
#elif (CAMERA_H == 180 )
    {OV7725_VOutSize     , 0x5a},
#elif (CAMERA_H == 240 )
    {OV7725_VOutSize     , 0x78},
#else

#endif

    {OV7725_EXHCH        , 0x00},
    {OV7725_GAM1         , 0x0c},
    {OV7725_GAM2         , 0x16},
    {OV7725_GAM3         , 0x2a},
    {OV7725_GAM4         , 0x4e},
    {OV7725_GAM5         , 0x61},
    {OV7725_GAM6         , 0x6f},
    {OV7725_GAM7         , 0x7b},
    {OV7725_GAM8         , 0x86},
    {OV7725_GAM9         , 0x8e},
    {OV7725_GAM10        , 0x97},
    {OV7725_GAM11        , 0xa4},
    {OV7725_GAM12        , 0xaf},
    {OV7725_GAM13        , 0xc5},
    {OV7725_GAM14        , 0xd7},
    {OV7725_GAM15        , 0xe8},
    {OV7725_SLOP         , 0x20},
    {OV7725_LC_RADI      , 0x00},
    {OV7725_LC_COEF      , 0x13},
    {OV7725_LC_XC        , 0x08},
    {OV7725_LC_COEFB     , 0x14},
    {OV7725_LC_COEFR     , 0x17},
    {OV7725_LC_CTR       , 0x05},
    {OV7725_BDBase       , 0x99},
    {OV7725_BDMStep      , 0x03},
    {OV7725_SDE          , 0x04},
    {OV7725_BRIGHT       , 0x00},
    {OV7725_CNST         , 75},
    {OV7725_SIGN         , 0x06},
    {OV7725_UVADJ0       , 0x11},
    {OV7725_UVADJ1       , 0x02},

};

uint8 ov7725_eagle_cfgnum = ARR_SIZE( ov7725_eagle_reg ) ; /*结构体数组成员数目*/


/*!
 *  @brief      鹰眼ov7725寄存器 初始化
 *  @return     初始化结果（0表示失败，1表示成功）
 *  @since      v5.0
 */
uint8 ov7725_eagle_reg_init(void)
{
    uint16 i = 0;
    uint8 Sensor_IDCode = 0;
    SCCB_GPIO_init();

    //OV7725_Delay_ms(50);
    if( 0 == SCCB_WriteByte ( OV7725_COM7, 0x80 ) ) /*复位sensor */
    {
        DEBUG_PRINTF("\n警告:SCCB写数据错误");
        return 0 ;
    }

    OV7725_EAGLE_Delay_ms(50);

    if( 0 == SCCB_ReadByte( &Sensor_IDCode, 1, OV7725_VER ) )    /* 读取sensor ID号*/
    {
        DEBUG_PRINTF("\n警告:读取ID失败");
        return 0;
    }
    DEBUG_PRINTF("\nGet ID success，SENSOR ID is 0x%x", Sensor_IDCode);
    DEBUG_PRINTF("\nConfig Register Number is %d ", ov7725_eagle_cfgnum);
    if(Sensor_IDCode == OV7725_ID)
    {
        for( i = 0 ; i < ov7725_eagle_cfgnum ; i++ )
        {
            if( 0 == SCCB_WriteByte(ov7725_eagle_reg[i].addr, ov7725_eagle_reg[i].val) )
            {
                DEBUG_PRINTF("\n警告:写寄存器0x%x失败", ov7725_eagle_reg[i].addr);
                return 0;
            }
        }
    }
    else
    {
        return 0;
    }
    DEBUG_PRINTF("\nOV7725 Register Config Success!");
    return 1;
}


