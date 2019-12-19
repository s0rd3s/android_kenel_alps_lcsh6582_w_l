#include "tpd.h"
#include <linux/interrupt.h>
#include <cust_eint.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>

#include "tpd_custom_msg2133.h"
#include "open_test_ANA1_B_shenyue.h"
#include "open_test_ANA1_shenyue.h"
#include "open_test_ANA2_B_shenyue.h"
#include "open_test_ANA2_shenyue.h"
#include "open_test_ANA3_shenyue.h"


#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>
#include <mach/mt_gpio.h>

#include "cust_gpio_usage.h"

//for dma mode
#include <linux/dma-mapping.h>
#include <linux/mm_types.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/vmalloc.h>

#include <linux/proc_fs.h>

extern struct  i2c_client *msg_i2c_client;

extern int HalTscrCDevWriteI2CSeq(u8 addr, u8* data, u16 size);
extern int HalTscrCReadI2CSeq(u8 addr, u8* read_data, u16 size);
extern void msg_dma_alloct();
extern void msg_dma_release();
extern u8 *g_dma_buff_va;
extern u8 *g_dma_buff_pa;

#ifdef WT_CTP_OPEN_SHORT_TEST
#include <linux/proc_fs.h>

u8 bItoTestDebug = 1;
s16  s16_raw_data_1[48] = {0};
s16  s16_raw_data_2[48] = {0};
s16  s16_raw_data_3[48] = {0};
u8 ito_test_keynum = 0;
u8 ito_test_dummynum = 0;
u8 ito_test_trianglenum = 0;
u8 ito_test_2r = 0;
u8 g_LTP = 1;	
uint16_t *open_1 = NULL;
uint16_t *open_1B = NULL;
uint16_t *open_2 = NULL;
uint16_t *open_2B = NULL;
uint16_t *open_3 = NULL;
u8 *MAP1 = NULL;
u8 *MAP2=NULL;
u8 *MAP3=NULL;
u8 *MAP40_1 = NULL;
u8 *MAP40_2 = NULL;
u8 *MAP40_3 = NULL;
u8 *MAP40_4 = NULL;
u8 *MAP41_1 = NULL;
u8 *MAP41_2 = NULL;
u8 *MAP41_3 = NULL;
u8 *MAP41_4 = NULL;


#define ITO_TEST_ADDR_TP  (0x4C>>1)
#define ITO_TEST_ADDR_REG (0xC4>>1)
#define REG_INTR_FIQ_MASK           0x04
#define FIQ_E_FRAME_READY_MASK      ( 1 << 8 )
#define MAX_CHNL_NUM (48)
#define BIT0  (1<<0)
#define BIT1  (1<<1)
#define BIT2  (1<<2)
#define BIT5  (1<<5)
#define BIT11 (1<<11)
#define BIT15 (1<<15)

//#define TP_OF_OFILM    (1)
#define TP_OF_SHENYUE    (9)


#define ITO_TEST_AUTHORITY 0777 

#define PROC_MSG_ITO_TESE      "ctp_test"
#define PROC_ITO_OPENTEST_DEBUG      "opentest"
#define PROC_ITO_SHORTTEST_DEBUG      "shorttest"
#define PROC_ITO_TEST_DEBUG_ON_OFF     "debug-on-off"
#define ITO_TEST_DEBUG_MUST(format, ...)	printk(KERN_ERR "ito_test ***" format "\n", ## __VA_ARGS__);mdelay(5)
#define ITO_TEST_DEBUG(format, ...) \
{ \
    if(bItoTestDebug) \
    { \
        printk(KERN_ERR "ito_test ***" format "\n", ## __VA_ARGS__); \
        mdelay(5); \
    } \
}
typedef enum
{
	ITO_TEST_OK = 0,
	ITO_TEST_FAIL,
	ITO_TEST_GET_TP_TYPE_ERROR,
} ITO_TEST_RET;
ITO_TEST_RET g_ito_test_ret;
int ito_test_i2c_read(U8 addr, U8* read_data, U16 size)//modify : 根据项目修改 msg_i2c_client
{
    int rc;
    U8 addr_before = msg_i2c_client->addr;
    msg_i2c_client->addr = addr;

    #ifdef MSG_DMA_MODE
    if(size>8&&NULL!=g_dma_buff_va)
    {
        int i = 0;
        msg_i2c_client->ext_flag = msg_i2c_client->ext_flag | I2C_DMA_FLAG ;
        rc = i2c_master_recv(msg_i2c_client, (unsigned char *)g_dma_buff_pa, size);
        for(i = 0; i < size; i++)
   		{
        	read_data[i] = g_dma_buff_va[i];
    	}
    }
    else
    {
        rc = i2c_master_recv(msg_i2c_client, read_data, size);
    }
    msg_i2c_client->ext_flag = msg_i2c_client->ext_flag & (~I2C_DMA_FLAG);	
    #else
    rc = i2c_master_recv(msg_i2c_client, read_data, size);
    #endif

    msg_i2c_client->addr = addr_before;
    if( rc < 0 )
    {
        ITO_TEST_DEBUG_MUST("ito_test_i2c_read error %d,addr=%d\n", rc,addr);
    }
    return rc;
}
int ito_test_i2c_write(U8 addr, U8* data, U16 size)//modify : 根据项目修改 msg_i2c_client
{
    int rc;
    U8 addr_before = msg_i2c_client->addr;
    msg_i2c_client->addr = addr;

#ifdef MSG_DMA_MODE
    if(size>8&&NULL!=g_dma_buff_va)
	{
	    int i = 0;
	    for(i=0;i<size;i++)
    	{
    		 g_dma_buff_va[i]=data[i];
    	}
		msg_i2c_client->ext_flag = msg_i2c_client->ext_flag | I2C_DMA_FLAG ;
		rc = i2c_master_send(msg_i2c_client, (unsigned char *)g_dma_buff_pa, size);
	}
	else
	{
		rc = i2c_master_send(msg_i2c_client, data, size);
	}
    msg_i2c_client->ext_flag = msg_i2c_client->ext_flag & (~I2C_DMA_FLAG);	
#else
    rc = i2c_master_send(msg_i2c_client, data, size);
#endif

    msg_i2c_client->addr = addr_before;
    if( rc < 0 )
    {
        ITO_TEST_DEBUG_MUST("ito_test_i2c_write error %d,addr = %d,data[0]=%d\n", rc, addr,data[0]);
    }
    return rc;
}
void ito_test_reset(void)//modify:根据项目修改
{
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	msleep(10);
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);  
	msleep(100);
	ITO_TEST_DEBUG("reset tp\n");
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
	msleep(200);
}
void ito_test_disable_irq(void)//modify:根据项目修改
{
	mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
}
void ito_test_enable_irq(void)//modify:根据项目修改
{
	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
}

void ito_test_set_iic_rate(u32 iicRate)//modify:根据平台修改,iic速率要求50K
{
    msg_i2c_client->timing = iicRate/1000;
}

void ito_test_WriteReg( u8 bank, u8 addr, u16 data )
{
    u8 tx_data[5] = {0x10, bank, addr, data & 0xFF, data >> 8};
    ito_test_i2c_write( ITO_TEST_ADDR_REG, &tx_data[0], 5 );
}
void ito_test_WriteReg8Bit( u8 bank, u8 addr, u8 data )
{
    u8 tx_data[4] = {0x10, bank, addr, data};
    ito_test_i2c_write ( ITO_TEST_ADDR_REG, &tx_data[0], 4 );
}
unsigned short ito_test_ReadReg( u8 bank, u8 addr )
{
    u8 tx_data[3] = {0x10, bank, addr};
    u8 rx_data[2] = {0};

    ito_test_i2c_write( ITO_TEST_ADDR_REG, &tx_data[0], 3 );
    ito_test_i2c_read ( ITO_TEST_ADDR_REG, &rx_data[0], 2 );
    return ( rx_data[1] << 8 | rx_data[0] );
}
u32 ito_test_get_TpType(void)
{
    u8 tx_data[3] = {0};
    u8 rx_data[4] = {0};
    u32 Major = 0, Minor = 0;

    ITO_TEST_DEBUG("GetTpType\n");
        
    tx_data[0] = 0x53;
    tx_data[1] = 0x00;
    tx_data[2] = 0x2A;
    ito_test_i2c_write(ITO_TEST_ADDR_TP, &tx_data[0], 3);
    mdelay(50);
    ito_test_i2c_read(ITO_TEST_ADDR_TP, &rx_data[0], 4);
    Major = (rx_data[1]<<8) + rx_data[0];
    Minor = (rx_data[3]<<8) + rx_data[2];

    ITO_TEST_DEBUG("***TpTypeMajor = %d ***\n", Major);
    ITO_TEST_DEBUG("***TpTypeMinor = %d ***\n", Minor);
    
    return Major;
    
}

u32 ito_test_choose_TpType(void)
{
    u32 tpType = 0;
    u8 i = 0;
    open_1 = NULL;
    open_1B = NULL;
    open_2 = NULL;
    open_2B = NULL;
    open_3 = NULL;
    MAP1 = NULL;
    MAP2 = NULL;
    MAP3 = NULL;
    MAP40_1 = NULL;
    MAP40_2 = NULL;
    MAP40_3 = NULL;
    MAP40_4 = NULL;
    MAP41_1 = NULL;
    MAP41_2 = NULL;
    MAP41_3 = NULL;
    MAP41_4 = NULL;
    ito_test_keynum = 0;
    ito_test_dummynum = 0;
    ito_test_trianglenum = 0;
    ito_test_2r = 0;

    for(i=0;i<10;i++)
    {
        tpType = ito_test_get_TpType();
        ITO_TEST_DEBUG("tpType=%d;i=%d;\n",tpType,i);
        if(TP_OF_SHENYUE==tpType)//modify:注意该项目tp数目
        {
            break;
        }
        else if(i<5)
        {
            mdelay(100);  
        }
        else
        {
            ito_test_reset();
        }
    }
  
     if(TP_OF_SHENYUE==tpType)
    {
        open_1 = open_1_shenyue;
        open_1B = open_1B_shenyue;
        open_2 = open_2_shenyue;
        open_2B = open_2B_shenyue;
        open_3 = open_3_shenyue;
        MAP1 = MAP1_shenyue;
        MAP2 = MAP2_shenyue;
        MAP3 = MAP3_shenyue;
        MAP40_1 = MAP40_1_shenyue;
        MAP40_2 = MAP40_2_shenyue;
        MAP40_3 = MAP40_3_shenyue;
        MAP40_4 = MAP40_4_shenyue;
        MAP41_1 = MAP41_1_shenyue;
        MAP41_2 = MAP41_2_shenyue;
        MAP41_3 = MAP41_3_shenyue;
        MAP41_4 = MAP41_4_shenyue;
        ito_test_keynum = NUM_KEY_SHENYUE;
        ito_test_dummynum = NUM_DUMMY_SHENYUE;
        ito_test_trianglenum = NUM_SENSOR_SHENYUE;
        ito_test_2r = ENABLE_2R_SHENYUE;
    }
    else
    {
        tpType = 0;
    }
    return tpType;
}
void ito_test_EnterSerialDebugMode(void)
{
    u8 data[5];

    data[0] = 0x53;
    data[1] = 0x45;
    data[2] = 0x52;
    data[3] = 0x44;
    data[4] = 0x42;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, &data[0], 5);

    data[0] = 0x37;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, &data[0], 1);

    data[0] = 0x35;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, &data[0], 1);

    data[0] = 0x71;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, &data[0], 1);
}
uint16_t ito_test_get_num( void )
{
    uint16_t    num_of_sensor,i;
    uint16_t 	RegValue1,RegValue2;
 
    num_of_sensor = 0;
        
    RegValue1 = ito_test_ReadReg( 0x11, 0x4A);
    ITO_TEST_DEBUG("ito_test_get_num,RegValue1=%d\n",RegValue1);
    if ( ( RegValue1 & BIT1) == BIT1 )
    {
    	RegValue1 = ito_test_ReadReg( 0x12, 0x0A);			
    	RegValue1 = RegValue1 & 0x0F;
    	
    	RegValue2 = ito_test_ReadReg( 0x12, 0x16);    		
    	RegValue2 = (( RegValue2 >> 1 ) & 0x0F) + 1;
    	
    	num_of_sensor = RegValue1 * RegValue2;
    }
	else
	{
	    for(i=0;i<4;i++)
	    {
	        num_of_sensor+=(ito_test_ReadReg( 0x12, 0x0A)>>(4*i))&0x0F;
	    }
	}
    ITO_TEST_DEBUG("ito_test_get_num,num_of_sensor=%d\n",num_of_sensor);
    return num_of_sensor;        
}
void ito_test_polling( void )
{
    uint16_t    reg_int = 0x0000;
    uint8_t     dbbus_tx_data[5];
    uint8_t     dbbus_rx_data[4];
    uint16_t    reg_value;


    reg_int = 0;

    ito_test_WriteReg( 0x13, 0x0C, BIT15 );       
    ito_test_WriteReg( 0x12, 0x14, (ito_test_ReadReg(0x12,0x14) | BIT0) );         
            
    ITO_TEST_DEBUG("polling start\n");
    while( ( reg_int & BIT0 ) == 0x0000 )
    {
        dbbus_tx_data[0] = 0x10;
        dbbus_tx_data[1] = 0x3D;
        dbbus_tx_data[2] = 0x18;
        ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 3);
        ito_test_i2c_read(ITO_TEST_ADDR_REG,  dbbus_rx_data, 2);
        reg_int = dbbus_rx_data[1];
    }
    ITO_TEST_DEBUG("polling end\n");
    reg_value = ito_test_ReadReg( 0x3D, 0x18 ); 
    ito_test_WriteReg( 0x3D, 0x18, reg_value & (~BIT0) );      
}
uint16_t ito_test_get_data_out( int16_t* s16_raw_data )
{
    uint8_t     i,dbbus_tx_data[8];
    uint16_t    raw_data[48]={0};
    uint16_t    num_of_sensor;
    uint16_t    reg_int;
    uint8_t		dbbus_rx_data[96]={0};
  
    num_of_sensor = ito_test_get_num();
    if(num_of_sensor>11)
    {
        ITO_TEST_DEBUG("danger,num_of_sensor=%d\n",num_of_sensor);
        return num_of_sensor;
    }

    reg_int = ito_test_ReadReg( 0x3d, REG_INTR_FIQ_MASK<<1 ); 
    ito_test_WriteReg( 0x3d, REG_INTR_FIQ_MASK<<1, (reg_int & (uint16_t)(~FIQ_E_FRAME_READY_MASK) ) ); 
    ito_test_polling();
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x13;
    dbbus_tx_data[2] = 0x40;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 3);
    mdelay(20);
    ito_test_i2c_read(ITO_TEST_ADDR_REG, &dbbus_rx_data[0], (num_of_sensor * 2));
    mdelay(100);
    for(i=0;i<num_of_sensor * 2;i++)
    {
        ITO_TEST_DEBUG("dbbus_rx_data[%d]=%d\n",i,dbbus_rx_data[i]);
    }
 
    reg_int = ito_test_ReadReg( 0x3d, REG_INTR_FIQ_MASK<<1 ); 
    ito_test_WriteReg( 0x3d, REG_INTR_FIQ_MASK<<1, (reg_int | (uint16_t)FIQ_E_FRAME_READY_MASK ) ); 


    for( i = 0; i < num_of_sensor; i++ )
    {
        raw_data[i] = ( dbbus_rx_data[ 2 * i + 1] << 8 ) | ( dbbus_rx_data[2 * i] );
        s16_raw_data[i] = ( int16_t )raw_data[i];
    }
    
    return(num_of_sensor);
}
void ito_test_send_data_in( uint8_t step )
{
    uint16_t	i;
    uint8_t 	dbbus_tx_data[512];
    uint16_t 	*Type1=NULL;        

    ITO_TEST_DEBUG("ito_test_send_data_in step=%d\n",step);
	if( step == 4 )
    {
        Type1 = &open_1[0];        
    }
    else if( step == 5 )
    {
        Type1 = &open_2[0];      	
    }
    else if( step == 6 )
    {
        Type1 = &open_3[0];      	
    }
    else if( step == 9 )
    {
        Type1 = &open_1B[0];        
    }
    else if( step == 10 )
    {
        Type1 = &open_2B[0];      	
    } 
     
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0x00;    
    for( i = 0; i <= 0x3E ; i++ )
    {
        dbbus_tx_data[3+2*i] = Type1[i] & 0xFF;
        dbbus_tx_data[4+2*i] = ( Type1[i] >> 8 ) & 0xFF;    	
    }
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 3+0x3F*2);
 
    dbbus_tx_data[2] = 0x7A * 2;
    for( i = 0x7A; i <= 0x7D ; i++ )
    {
        dbbus_tx_data[3+2*(i-0x7A)] = 0;
        dbbus_tx_data[4+2*(i-0x7A)] = 0;    	    	
    }
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 3+8);  
    
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x12;
      
    dbbus_tx_data[2] = 5 * 2;
    dbbus_tx_data[3] = Type1[128+5] & 0xFF;
    dbbus_tx_data[4] = ( Type1[128+5] >> 8 ) & 0xFF;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 5);
    
    dbbus_tx_data[2] = 0x0B * 2;
    dbbus_tx_data[3] = Type1[128+0x0B] & 0xFF;
    dbbus_tx_data[4] = ( Type1[128+0x0B] >> 8 ) & 0xFF;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 5);
    
    dbbus_tx_data[2] = 0x12 * 2;
    dbbus_tx_data[3] = Type1[128+0x12] & 0xFF;
    dbbus_tx_data[4] = ( Type1[128+0x12] >> 8 ) & 0xFF;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 5);
    
    dbbus_tx_data[2] = 0x15 * 2;
    dbbus_tx_data[3] = Type1[128+0x15] & 0xFF;
    dbbus_tx_data[4] = ( Type1[128+0x15] >> 8 ) & 0xFF;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 5);        

 #if 1//for AC mod --showlo
    dbbus_tx_data[1] = 0x13;
    dbbus_tx_data[2] = 0x12 * 2;
    dbbus_tx_data[3] = 0X30;
    dbbus_tx_data[4] = 0X30;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 5);        

    
    dbbus_tx_data[2] = 0x14 * 2;
    dbbus_tx_data[3] = 0X30;
    dbbus_tx_data[4] = 0X30;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 5);     

    
    dbbus_tx_data[1] = 0x12;
    for (i = 0x0D; i <= 0x10;i++ )//for AC noise(++)
    {
        dbbus_tx_data[2] = i * 2;
        dbbus_tx_data[3] = Type1[128+i] & 0xFF;
        dbbus_tx_data[4] = ( Type1[128+i] >> 8 ) & 0xFF;
        ito_test_i2c_write( ITO_TEST_ADDR_REG,  dbbus_tx_data,5 );  
    }

    for (i = 0x16; i <= 0x18; i++)//for AC noise
    {
        dbbus_tx_data[2] = i * 2;
        dbbus_tx_data[3] = Type1[128+i] & 0xFF;
        dbbus_tx_data[4] = ( Type1[128+i] >> 8 ) & 0xFF;
        ito_test_i2c_write( ITO_TEST_ADDR_REG, dbbus_tx_data,5 );  
    }
#endif

}

void ito_test_set_v( uint8_t Enable, uint8_t Prs)	
{
    uint16_t    u16RegValue;        
    
    
    u16RegValue = ito_test_ReadReg( 0x12, 0x08);   
    u16RegValue = u16RegValue & 0xF1; 							
    if ( Prs == 0 )
    {
    	ito_test_WriteReg( 0x12, 0x08, u16RegValue| 0x0C); 		
    }
    else if ( Prs == 1 )
    {
    	ito_test_WriteReg( 0x12, 0x08, u16RegValue| 0x0E); 		     	
    }
    else
    {
    	ito_test_WriteReg( 0x12, 0x08, u16RegValue| 0x02); 			
    }    
    
    if ( Enable )
    {
        u16RegValue = ito_test_ReadReg( 0x11, 0x06);    
        ito_test_WriteReg( 0x11, 0x06, u16RegValue| 0x03);   	
    }
    else
    {
        u16RegValue = ito_test_ReadReg( 0x11, 0x06);    
        u16RegValue = u16RegValue & 0xFC;					
        ito_test_WriteReg( 0x11, 0x06, u16RegValue);         
    }

}

void ito_test_set_c( uint8_t Csub_Step )
{
    uint8_t i;
    uint8_t dbbus_tx_data[MAX_CHNL_NUM+3];
    uint8_t HighLevel_Csub = false;
    uint8_t Csub_new;
     
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;        
    dbbus_tx_data[2] = 0x84;        
    for( i = 0; i < MAX_CHNL_NUM; i++ )
    {
		Csub_new = Csub_Step;        
        HighLevel_Csub = false;   
        if( Csub_new > 0x1F )
        {
            Csub_new = Csub_new - 0x14;
            HighLevel_Csub = true;
        }
           
        dbbus_tx_data[3+i] =    Csub_new & 0x1F;        
        if( HighLevel_Csub == true )
        {
            dbbus_tx_data[3+i] |= BIT5;
        }
    }
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, MAX_CHNL_NUM+3);

    dbbus_tx_data[2] = 0xB4;        
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, MAX_CHNL_NUM+3);
}

void ito_test_sw( void )
{
    ito_test_WriteReg( 0x11, 0x00, 0xFFFF );
    ito_test_WriteReg( 0x11, 0x00, 0x0000 );
    mdelay( 50 );
}


void Disable_noise_detect( void )
{
    ito_test_WriteReg8Bit( 0x13, 0x02, (ito_test_ReadReg(0x13,0x01) & ~( BIT2|BIT0|BIT1 ) ) );
}

void ito_test_first( uint8_t item_id , int16_t* s16_raw_data)		
{
	uint8_t     result = 0,loop;
	uint8_t     dbbus_tx_data[9];
	uint8_t     i,j;
    int16_t     s16_raw_data_tmp[48]={0};
	uint8_t     num_of_sensor, num_of_sensor2,total_sensor;
	uint16_t	u16RegValue;
    uint8_t 	*pMapping=NULL;
    
    
	num_of_sensor = 0;
	num_of_sensor2 = 0;	
	
    ITO_TEST_DEBUG("ito_test_first item_id=%d\n",item_id);
	ito_test_WriteReg( 0x0F, 0xE6, 0x01 );

	ito_test_WriteReg( 0x1E, 0x24, 0x0500 );
	ito_test_WriteReg( 0x1E, 0x2A, 0x0000 );
	ito_test_WriteReg( 0x1E, 0xE6, 0x6E00 );
	ito_test_WriteReg( 0x1E, 0xE8, 0x0071 );
	    
    if ( item_id == 40 )    			
    {
        pMapping = &MAP1[0];
        if ( ito_test_2r )
		{
			total_sensor = ito_test_trianglenum/2; 
		}
		else
		{
		    total_sensor = ito_test_trianglenum/2 + ito_test_keynum + ito_test_dummynum;
		}
    }
    else if( item_id == 41 )    		
    {
        pMapping = &MAP2[0];
        if ( ito_test_2r )
		{
			total_sensor = ito_test_trianglenum/2; 
		}
		else
		{
		    total_sensor = ito_test_trianglenum/2 + ito_test_keynum + ito_test_dummynum;
		}
    }
    else if( item_id == 42 )    		
    {
        pMapping = &MAP3[0];      
        total_sensor =  ito_test_trianglenum + ito_test_keynum+ ito_test_dummynum; 
    }
        	    
	    
	loop = 1;
	if ( item_id != 42 )
	{
	    if(total_sensor>11)
        {
            loop = 2;
        }
	}	
    ITO_TEST_DEBUG("loop=%d\n",loop);
	for ( i = 0; i < loop; i++ )
	{
		if ( i == 0 )
		{
			ito_test_send_data_in( item_id - 36 );
		}
		else
		{ 
			if ( item_id == 40 ) 
				ito_test_send_data_in( 9 );
			else 		
				ito_test_send_data_in( 10 );
		}
	
	    Disable_noise_detect();
        
		ito_test_set_v(1,0);    
		u16RegValue = ito_test_ReadReg( 0x11, 0x0E);    			
		ito_test_WriteReg( 0x11, 0x0E, u16RegValue | BIT11 );				 		
	
		if ( g_LTP == 1 )
	    	ito_test_set_c( 32 );	    	
		else	    	
	    	ito_test_set_c( 0 );
	    
		ito_test_sw();
		
		if ( i == 0 )	 
        {      
            num_of_sensor=ito_test_get_data_out(  s16_raw_data_tmp );
            ITO_TEST_DEBUG("num_of_sensor=%d;\n",num_of_sensor);
        }
		else	
        {      
            num_of_sensor2=ito_test_get_data_out(  &s16_raw_data_tmp[num_of_sensor] );
            ITO_TEST_DEBUG("num_of_sensor=%d;num_of_sensor2=%d\n",num_of_sensor,num_of_sensor2);
        }
	}
    for ( j = 0; j < total_sensor ; j ++ )
	{
		if ( g_LTP == 1 )
			s16_raw_data[pMapping[j]] = s16_raw_data_tmp[j] + 4096;
		else
			s16_raw_data[pMapping[j]] = s16_raw_data_tmp[j];	
	}	

	return;
}


ITO_TEST_RET ito_test_second (u8 item_id)
{
	u8 i = 0;
    
	s32  s16_raw_data_jg_tmp1 = 0;
	s32  s16_raw_data_jg_tmp2 = 0;
	s32  jg_tmp1_avg_Th_max =0;
	s32  jg_tmp1_avg_Th_min =0;
	s32  jg_tmp2_avg_Th_max =0;
	s32  jg_tmp2_avg_Th_min =0;

	u8  Th_Tri = 40;        
	u8  Th_bor = 40;        

	if ( item_id == 40 )    			
    {
        for (i=0; i<(ito_test_trianglenum/2)-2; i++)
        {
			s16_raw_data_jg_tmp1 += s16_raw_data_1[MAP40_1[i]];
		}
		for (i=0; i<2; i++)
        {
			s16_raw_data_jg_tmp2 += s16_raw_data_1[MAP40_2[i]];
		}
    }
    else if( item_id == 41 )    		
    {
        for (i=0; i<(ito_test_trianglenum/2)-2; i++)
        {
			s16_raw_data_jg_tmp1 += s16_raw_data_2[MAP41_1[i]];
		}
		for (i=0; i<2; i++)
        {
			s16_raw_data_jg_tmp2 += s16_raw_data_2[MAP41_2[i]];
		}
    }

	    jg_tmp1_avg_Th_max = (s16_raw_data_jg_tmp1 / ((ito_test_trianglenum/2)-2)) * ( 100 + Th_Tri) / 100 ;
	    jg_tmp1_avg_Th_min = (s16_raw_data_jg_tmp1 / ((ito_test_trianglenum/2)-2)) * ( 100 - Th_Tri) / 100 ;
        jg_tmp2_avg_Th_max = (s16_raw_data_jg_tmp2 / 2) * ( 100 + Th_bor) / 100 ;
	    jg_tmp2_avg_Th_min = (s16_raw_data_jg_tmp2 / 2 ) * ( 100 - Th_bor) / 100 ;
	
        ITO_TEST_DEBUG("item_id=%d;sum1=%d;max1=%d;min1=%d;sum2=%d;max2=%d;min2=%d\n",item_id,s16_raw_data_jg_tmp1,jg_tmp1_avg_Th_max,jg_tmp1_avg_Th_min,s16_raw_data_jg_tmp2,jg_tmp2_avg_Th_max,jg_tmp2_avg_Th_min);

	if ( item_id == 40 ) 
	{
		for (i=0; i<(ito_test_trianglenum/2)-2; i++)
	    {
			if (s16_raw_data_1[MAP40_1[i]] > jg_tmp1_avg_Th_max || s16_raw_data_1[MAP40_1[i]] < jg_tmp1_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		for (i=0; i<2; i++)
	    {
			if (s16_raw_data_1[MAP40_2[i]] > jg_tmp2_avg_Th_max || s16_raw_data_1[MAP40_2[i]] < jg_tmp2_avg_Th_min) 
				return ITO_TEST_FAIL;
		} 
	}

	if ( item_id == 41 ) 
	{
		for (i=0; i<(ito_test_trianglenum/2)-2; i++)
	    {
			if (s16_raw_data_2[MAP41_1[i]] > jg_tmp1_avg_Th_max || s16_raw_data_2[MAP41_1[i]] < jg_tmp1_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		for (i=0; i<2; i++)
	    {
			if (s16_raw_data_2[MAP41_2[i]] > jg_tmp2_avg_Th_max || s16_raw_data_2[MAP41_2[i]] < jg_tmp2_avg_Th_min) 
				return ITO_TEST_FAIL;
		} 
	}

	return ITO_TEST_OK;
	
}
ITO_TEST_RET ito_test_second_2r (u8 item_id)
{
	u8 i = 0;
    
	s32  s16_raw_data_jg_tmp1 = 0;
	s32  s16_raw_data_jg_tmp2 = 0;
	s32  s16_raw_data_jg_tmp3 = 0;
	s32  s16_raw_data_jg_tmp4 = 0;
	
	s32  jg_tmp1_avg_Th_max =0;
	s32  jg_tmp1_avg_Th_min =0;
	s32  jg_tmp2_avg_Th_max =0;
	s32  jg_tmp2_avg_Th_min =0;
	s32  jg_tmp3_avg_Th_max =0;
	s32  jg_tmp3_avg_Th_min =0;
	s32  jg_tmp4_avg_Th_max =0;
	s32  jg_tmp4_avg_Th_min =0;

	u8  Th_Tri = 40;    // non-border threshold    
	u8  Th_bor = 40;    // border threshold    

	if ( item_id == 40 )    			
    {
        for (i=0; i<(ito_test_trianglenum/4)-2; i++)
        {
			s16_raw_data_jg_tmp1 += s16_raw_data_1[MAP40_1[i]];  //first region: non-border 
		}
		for (i=0; i<2; i++)
        {
			s16_raw_data_jg_tmp2 += s16_raw_data_1[MAP40_2[i]];  //first region: border
		}

		for (i=0; i<(ito_test_trianglenum/4)-2; i++)
        {
			s16_raw_data_jg_tmp3 += s16_raw_data_1[MAP40_3[i]];  //second region: non-border
		}
		for (i=0; i<2; i++)
        {
			s16_raw_data_jg_tmp4 += s16_raw_data_1[MAP40_4[i]];  //second region: border
		}
    }



	
    else if( item_id == 41 )    		
    {
        for (i=0; i<(ito_test_trianglenum/4)-2; i++)
        {
			s16_raw_data_jg_tmp1 += s16_raw_data_2[MAP41_1[i]];  //first region: non-border
		}
		for (i=0; i<2; i++)
        {
			s16_raw_data_jg_tmp2 += s16_raw_data_2[MAP41_2[i]];  //first region: border
		}
		for (i=0; i<(ito_test_trianglenum/4)-2; i++)
        {
			s16_raw_data_jg_tmp3 += s16_raw_data_2[MAP41_3[i]];  //second region: non-border
		}
		for (i=0; i<2; i++)
        {
			s16_raw_data_jg_tmp4 += s16_raw_data_2[MAP41_4[i]];  //second region: border
		}
    }

	    jg_tmp1_avg_Th_max = (s16_raw_data_jg_tmp1 / ((ito_test_trianglenum/4)-2)) * ( 100 + Th_Tri) / 100 ;
	    jg_tmp1_avg_Th_min = (s16_raw_data_jg_tmp1 / ((ito_test_trianglenum/4)-2)) * ( 100 - Th_Tri) / 100 ;
        jg_tmp2_avg_Th_max = (s16_raw_data_jg_tmp2 / 2) * ( 100 + Th_bor) / 100 ;
	    jg_tmp2_avg_Th_min = (s16_raw_data_jg_tmp2 / 2) * ( 100 - Th_bor) / 100 ;
		jg_tmp3_avg_Th_max = (s16_raw_data_jg_tmp3 / ((ito_test_trianglenum/4)-2)) * ( 100 + Th_Tri) / 100 ;
	    jg_tmp3_avg_Th_min = (s16_raw_data_jg_tmp3 / ((ito_test_trianglenum/4)-2)) * ( 100 - Th_Tri) / 100 ;
        jg_tmp4_avg_Th_max = (s16_raw_data_jg_tmp4 / 2) * ( 100 + Th_bor) / 100 ;
	    jg_tmp4_avg_Th_min = (s16_raw_data_jg_tmp4 / 2) * ( 100 - Th_bor) / 100 ;
		
	
        ITO_TEST_DEBUG("item_id=%d;sum1=%d;max1=%d;min1=%d;sum2=%d;max2=%d;min2=%d;sum3=%d;max3=%d;min3=%d;sum4=%d;max4=%d;min4=%d;\n",item_id,s16_raw_data_jg_tmp1,jg_tmp1_avg_Th_max,jg_tmp1_avg_Th_min,s16_raw_data_jg_tmp2,jg_tmp2_avg_Th_max,jg_tmp2_avg_Th_min,s16_raw_data_jg_tmp3,jg_tmp3_avg_Th_max,jg_tmp3_avg_Th_min,s16_raw_data_jg_tmp4,jg_tmp4_avg_Th_max,jg_tmp4_avg_Th_min);




	if ( item_id == 40 ) 
	{
		for (i=0; i<(ito_test_trianglenum/4)-2; i++)
	    {
			if (s16_raw_data_1[MAP40_1[i]] > jg_tmp1_avg_Th_max || s16_raw_data_1[MAP40_1[i]] < jg_tmp1_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		
		for (i=0; i<2; i++)
	    {
			if (s16_raw_data_1[MAP40_2[i]] > jg_tmp2_avg_Th_max || s16_raw_data_1[MAP40_2[i]] < jg_tmp2_avg_Th_min) 
				return ITO_TEST_FAIL;
		} 
		
		for (i=0; i<(ito_test_trianglenum/4)-2; i++)
	    {
			if (s16_raw_data_1[MAP40_3[i]] > jg_tmp3_avg_Th_max || s16_raw_data_1[MAP40_3[i]] < jg_tmp3_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		
		for (i=0; i<2; i++)
	    {
			if (s16_raw_data_1[MAP40_4[i]] > jg_tmp4_avg_Th_max || s16_raw_data_1[MAP40_4[i]] < jg_tmp4_avg_Th_min) 
				return ITO_TEST_FAIL;
		} 
	}

	if ( item_id == 41 ) 
	{
		for (i=0; i<(ito_test_trianglenum/4)-2; i++)
	    {
			if (s16_raw_data_2[MAP41_1[i]] > jg_tmp1_avg_Th_max || s16_raw_data_2[MAP41_1[i]] < jg_tmp1_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		
		for (i=0; i<2; i++)
	    {
			if (s16_raw_data_2[MAP41_2[i]] > jg_tmp2_avg_Th_max || s16_raw_data_2[MAP41_2[i]] < jg_tmp2_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		
		for (i=0; i<(ito_test_trianglenum/4)-2; i++)
	    {
			if (s16_raw_data_2[MAP41_3[i]] > jg_tmp3_avg_Th_max || s16_raw_data_2[MAP41_3[i]] < jg_tmp3_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		
		for (i=0; i<2; i++)
	    {
			if (s16_raw_data_2[MAP41_4[i]] > jg_tmp4_avg_Th_max || s16_raw_data_2[MAP41_4[i]] < jg_tmp4_avg_Th_min) 
				return ITO_TEST_FAIL;
		} 

	}

	return ITO_TEST_OK;
	
}
ITO_TEST_RET ito_test_interface(void)
{
    ITO_TEST_RET ret = ITO_TEST_OK;
    uint16_t i = 0;
#ifdef MSG_DMA_MODE
    msg_dma_alloct(); //msg_dma_alloct
#endif
    ito_test_set_iic_rate(50000);
    ITO_TEST_DEBUG("start\n");
    ito_test_disable_irq();
	ito_test_reset();
    if(!ito_test_choose_TpType())
    {
        ITO_TEST_DEBUG("choose tpType fail\n");
        ret = ITO_TEST_GET_TP_TYPE_ERROR;
        goto ITO_TEST_END;
    }
    ito_test_EnterSerialDebugMode();
    mdelay(100);
    ITO_TEST_DEBUG("EnterSerialDebugMode\n");
    ito_test_WriteReg8Bit ( 0x0F, 0xE6, 0x01 );
    ito_test_WriteReg ( 0x3C, 0x60, 0xAA55 );
    ITO_TEST_DEBUG("stop mcu and disable watchdog V.005\n");   
    mdelay(50);
    
	for(i = 0;i < 48;i++)
	{
		s16_raw_data_1[i] = 0;
		s16_raw_data_2[i] = 0;
		s16_raw_data_3[i] = 0;
	}	
	
    ito_test_first(40, s16_raw_data_1);
    ITO_TEST_DEBUG("40 get s16_raw_data_1\n");
    if(ito_test_2r)
    {
        ret=ito_test_second_2r(40);
    }
    else
    {
        ret=ito_test_second(40);
    }
    if(ITO_TEST_FAIL==ret)
    {
        goto ITO_TEST_END;
    }
    
    ito_test_first(41, s16_raw_data_2);
    ITO_TEST_DEBUG("41 get s16_raw_data_2\n");
    if(ito_test_2r)
    {
        ret=ito_test_second_2r(41);
    }
    else
    {
        ret=ito_test_second(41);
    }
    if(ITO_TEST_FAIL==ret)
    {
        goto ITO_TEST_END;
    }
    
    ito_test_first(42, s16_raw_data_3);
    ITO_TEST_DEBUG("42 get s16_raw_data_3\n");
    
    ITO_TEST_END:
#ifdef MSG_DMA_MODE
    msg_dma_release();
#endif
    ito_test_set_iic_rate(100000);
	ito_test_reset();
    ito_test_enable_irq();
    ITO_TEST_DEBUG("end\n");
    return ret;
}
int ito_opentest_proc_read_debug(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int cnt= 0;
    int result = 0;
    g_ito_test_ret = ito_test_interface();
    if(ITO_TEST_OK==g_ito_test_ret)
    {
        ITO_TEST_DEBUG_MUST("ITO_TEST_OK");
		result =1;
    }
    else if(ITO_TEST_FAIL==g_ito_test_ret)
    {
        ITO_TEST_DEBUG_MUST("ITO_TEST_FAIL");
		result =0;
    }
    else if(ITO_TEST_GET_TP_TYPE_ERROR==g_ito_test_ret)
    {
        ITO_TEST_DEBUG_MUST("ITO_TEST_GET_TP_TYPE_ERROR");
		result =0;
    }
	   
    *eof = 1;
    return sprintf(&page[0], "result=%d", result); //cnt
}

int ito_opentest_proc_write_debug(struct file *file, const char *buffer, unsigned long count, void *data)
{    
    u16 i = 0;
    mdelay(5);
    ITO_TEST_DEBUG_MUST("ito_test_ret = %d",g_ito_test_ret);
    mdelay(5);
    for(i=0;i<48;i++)
    {
        ITO_TEST_DEBUG_MUST("data_1[%d]=%d;\n",i,s16_raw_data_1[i]);
    }
    mdelay(5);
    for(i=0;i<48;i++)
    {
        ITO_TEST_DEBUG_MUST("data_2[%d]=%d;\n",i,s16_raw_data_2[i]);
    }
    mdelay(5);
    for(i=0;i<48;i++)
    {
        ITO_TEST_DEBUG_MUST("data_3[%d]=%d;\n",i,s16_raw_data_3[i]);
    }
    mdelay(5);
    return count;
}
int ito_shorttest_proc_read_debug(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    MSG_DEBUG("shorttest read");

    if(off > 0)
        return 0;

    return sprintf(&page[0], "notest"); //cnt;
}

int ito_shorttest_proc_write_debug(struct file *file, const char *buffer, unsigned long count, void *data)
{
    MSG_DEBUG("tpd_shorttest write");
    return -1;
}

int ito_test_proc_read_debug_on_off(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int cnt= 0;
    
    bItoTestDebug = 1;
    ITO_TEST_DEBUG_MUST("on debug bItoTestDebug = %d",bItoTestDebug);
    
    *eof = 1;
    return cnt;
}

int ito_test_proc_write_debug_on_off(struct file *file, const char *buffer, unsigned long count, void *data)
{    
    bItoTestDebug = 0;
    ITO_TEST_DEBUG_MUST("off debug bItoTestDebug = %d",bItoTestDebug);
    return count;
}

void ito_test_create_entry(void)//modify: 该函数在probe函数中调用
{
	struct proc_dir_entry *msg_ito_test = NULL;
	struct proc_dir_entry *opentest = NULL;
	struct proc_dir_entry *shorttest = NULL;
	struct proc_dir_entry *debug_on_off = NULL;

    msg_ito_test = proc_mkdir(PROC_MSG_ITO_TESE, NULL);
    opentest = create_proc_entry(PROC_ITO_OPENTEST_DEBUG, ITO_TEST_AUTHORITY, msg_ito_test);
	shorttest = create_proc_entry(PROC_ITO_SHORTTEST_DEBUG, ITO_TEST_AUTHORITY, msg_ito_test);
    debug_on_off= create_proc_entry(PROC_ITO_TEST_DEBUG_ON_OFF, ITO_TEST_AUTHORITY, msg_ito_test);

    if (NULL==opentest) 
    {
        ITO_TEST_DEBUG_MUST("create_proc_entry ITO TEST DEBUG failed\n");
    } 
    else 
    {
        opentest->read_proc = ito_opentest_proc_read_debug;
        opentest->write_proc = ito_opentest_proc_write_debug;
        ITO_TEST_DEBUG_MUST("create_proc_entry ITO TEST DEBUG OK\n");
    }
    if (NULL==shorttest) 
    {
        ITO_TEST_DEBUG_MUST("create_proc_entry ITO TEST DEBUG failed\n");
    } 
    else 
    {
        shorttest->read_proc = ito_shorttest_proc_read_debug;
        shorttest->write_proc = ito_shorttest_proc_write_debug;
        ITO_TEST_DEBUG_MUST("create_proc_entry ITO TEST DEBUG OK\n");
    }

    if (NULL==debug_on_off) 
    {
        ITO_TEST_DEBUG_MUST("create_proc_entry ITO TEST ON OFF failed\n");
    } 
    else 
    {
        debug_on_off->read_proc = ito_test_proc_read_debug_on_off;
        debug_on_off->write_proc = ito_test_proc_write_debug_on_off;
        ITO_TEST_DEBUG_MUST("create_proc_entry ITO TEST ON OFF OK\n");
    }
}
#endif



#ifdef WT_CTP_GESTURE_SUPPORT

#define MSG_GESTURE_FUNCTION_DOUBLECLICK_FLAG  0x01    ///0000 0001
#define MSG_GESTURE_FUNCTION_UPDIRECT_FLAG     0x02    ///0000 0010
#define MSG_GESTURE_FUNCTION_DOWNDIRECT_FLAG   0x04    ///0000 0100
#define MSG_GESTURE_FUNCTION_LEFTDIRECT_FLAG   0x08    ///0000 1000
#define MSG_GESTURE_FUNCTION_RIGHTDIRECT_FLAG  0x10    ///0001 0000

u8 tpd_gesture_flag = 0;////if 1,enter gesture mode success;

///if 1; the tp return mode is this mode 
u8 tpd_gesture_double_click_mode = 0;
u8 tpd_gesture_up_direct_mode = 0;
u8 tpd_gesture_down_direct_mode = 0;
u8 tpd_gesture_left_direct_mode = 0;
u8 tpd_gesture_right_direct_mode = 0;

u8 set_gesture_flag = 0;  

/////1:want to open this mode
u8 set_gesture_double_click_mode = 0;
u8 set_gesture_up_direct_mode = 0;
u8 set_gesture_down_direct_mode = 0;
u8 set_gesture_left_direct_mode = 0; 
u8 set_gesture_right_direct_mode = 0;

////right_flag | left_flag | down_flag | up_flag | doubleclick_flag
u8 set_gesture_mode = 0;

char gtp_gesture_value = 'Z';
char gtp_gesture_onoff = '0';
char gtp_glove_onoff = '0';
const char gtp_gesture_type[]=GTP_GESTURE_TPYE_STR; 	 
const char gtp_glove_support_flag=GTP_GLOVE_SUPPORT_ONOFF;
const char gtp_gesture_support_flag=GTP_GESTURE_SUPPORT_ONOFF;
const char gtp_verson[] =GTP_DRIVER_VERSION;
char gtp_glove_support_flag_changed=NULL;;
char gtp_gesture_support_flag_changed=NULL;

void msg_OpenGestureFunction( int g_Mode )
 {
	 unsigned char dbbus_tx_data[3];
	 unsigned char dbbus_rx_data[2] = {0};
 
#ifdef MSG_DMA_MODE
	 msg_dma_alloct();
#endif
 
	 /**********open command*********/
	 dbbus_tx_data[0] = 0x58;
	 
	 dbbus_tx_data[1] = 0x00;
	 /*
	 0000 0001 DoubleClick
	 0000 0010 Up Direction
	 0000 0100 Down Direction
	 0000 1000 Left Direction
	 0001 0000 Right Direction
	 0001 1111 All Of Five Funciton
	 */
	 dbbus_tx_data[2] = (0xFF&g_Mode);
	 
	 MSG_DEBUG("***msg_OpenGestureFunction MSG_Gesture_Function_type = %x ***\n", dbbus_tx_data[2]);
	 if(
		 (dbbus_tx_data[2] >= 0x01)&&
		 (dbbus_tx_data[2] <= 0x1F)
		 )
	 {
		 HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_tx_data[0], 3);//
		 tpd_gesture_flag = 1;
		 MSG_DEBUG("***msg_OpenGestureFunction write success***\n");
	 }
	 else
	 {
		 MSG_DEBUG("***msg_OpenGestureFunction :the command is wrong!***\n");
	 }
	 /**********open command*********/
#ifdef MSG_DMA_MODE
	 msg_dma_release();
#endif
 
 }
 void msg_CloseGestureFunction( void )
 {
	 unsigned char dbbus_tx_data[3];
	 unsigned char dbbus_rx_data[2] = {0};
	#ifdef MSG_DMA_MODE
	 msg_dma_alloct();
	#endif
 
	 tpd_gesture_flag = 0;
 
	 /*******close command********/
	 dbbus_tx_data[0] = 0x59;
	 
	 dbbus_tx_data[1] = 0x00;
	 //close command is 0x00
	 dbbus_tx_data[2] = 0x00;
	 HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_tx_data[0], 3);
	 /*******close command********/
	
	#ifdef MSG_DMA_MODE
	 msg_dma_release();
	#endif
 
 }

int proc_gesture_data_read(char *page, char **start, off_t off, int count, int *eof, void *data)
  { 
	 //GTP_INFO("proc_gesture_data_read=%d\n",gtp_gesture_value);
	 return sprintf(page,"%c\n",gtp_gesture_value);	 
  }
  
 int proc_gesture_data_write(struct file *file, const char *buffer, unsigned long count, void *data)
  {    
	 sscanf(buffer,"%c",&gtp_gesture_value);	 
	  return count;
  }
  
int proc_gesture_type_read(char *page, char **start, off_t off, int count, int *eof, void *data)
  {
	  
	  return sprintf(page,"%s\n",gtp_gesture_type);
	  
  }
  
int proc_gesture_type_write(struct file *file, const char *buffer, unsigned long count, void *data)
  {    
	  
	  return -1;
  }
  
int proc_gesture_onoff_read(char *page, char **start, off_t off, int count, int *eof, void *data)
  {
	  if(NULL==gtp_gesture_support_flag_changed) return sprintf(page,"%c\n",gtp_gesture_support_flag);
	  else return sprintf(page,"%c\n",gtp_gesture_onoff); 
 
  }
  
int proc_gesture_onoff_write(struct file *file, const char *buffer, unsigned long count, void *data)
  {    
	  sscanf(buffer,"%c",&gtp_gesture_onoff);
	  gtp_gesture_support_flag_changed=1;
	   return count;
  }
int proc_glove_onoff_read(char *page, char **start, off_t off, int count, int *eof, void *data)
  {
	  if(NULL==gtp_glove_support_flag_changed) return sprintf(page,"%c\n",gtp_glove_support_flag);
	 else return sprintf(page,"%c\n",gtp_glove_onoff);
 
  }
  
  int proc_glove_onoff_write(struct file *file, const char *buffer, unsigned long count, void *data)
  {    
	  sscanf(buffer,"%c",&gtp_glove_onoff);   
	  gtp_glove_support_flag_changed=1;
	  //gtp_reset_guitar(i2c_client_point, 20);
	 // msleep(100);
	 // gtp_send_cfg(i2c_client_point);
	  
	  return count;
  }
 
int proc_version_gesture_read(char *page, char **start, off_t off, int count, int *eof, void *data)
	 {
		 return sprintf(page,"%s\n",gtp_verson);

	 }
	 
int proc_version_gesture_write(struct file *file, const char *buffer, unsigned long count, void *data)
	 {	  
		 return count;
	 }

 void tp_Gesture_Fucntion_Proc_File(void)
  {  
		   struct proc_dir_entry *ctp_device_proc = NULL;
		   struct proc_dir_entry *ctp_gesture_var_proc = NULL; 
		   struct proc_dir_entry *ctp_gesture_type_proc = NULL;
		   struct proc_dir_entry *ctp_gesture_onoff_proc = NULL;
		   struct proc_dir_entry *ctp_glove_onoff_proc = NULL;	   
		   struct proc_dir_entry *ctp_version_proc = NULL;
			#define CTP_GESTURE_FUNCTION_AUTHORITY_PROC 0777  
			 
		   ctp_device_proc = proc_mkdir("touchscreen_feature", NULL);
		   
		   ctp_gesture_var_proc = create_proc_entry("gesture_data", 0444, ctp_device_proc);
		   if (ctp_gesture_var_proc == NULL) 
		   {
        MSG_DEBUG("ctp_gesture_var_proc create failed\n");
		   } 
		   else 
		   {
			   ctp_gesture_var_proc->read_proc = proc_gesture_data_read;
			   ctp_gesture_var_proc->write_proc = proc_gesture_data_write;
        MSG_DEBUG("ctp_gesture_var_proc create success\n");
		   }
	  
		   ctp_gesture_type_proc = create_proc_entry("gesture_type", 0444, ctp_device_proc);
		   if (ctp_gesture_type_proc == NULL) 
		   {
        MSG_DEBUG("ctp_gesture_type_proc create failed\n");
		   } 
		   else 
		   {
			   ctp_gesture_type_proc->read_proc = proc_gesture_type_read;
			   ctp_gesture_type_proc->write_proc = proc_gesture_type_write;
        MSG_DEBUG("ctp_gesture_type_proc create success\n");
		   }
		   
		   ctp_gesture_onoff_proc = create_proc_entry("gesture_onoff", 0666, ctp_device_proc);
		   if (ctp_gesture_onoff_proc == NULL) 
		   {
        MSG_DEBUG("ctp_gesture_onoff_proc create failed\n");
		   } 
		   else 
		   {
			   ctp_gesture_onoff_proc->read_proc = proc_gesture_onoff_read;
			   ctp_gesture_onoff_proc->write_proc = proc_gesture_onoff_write;
        MSG_DEBUG("ctp_gesture_onoff_proc create  success\n");
		   }  
		   
		   ctp_glove_onoff_proc = create_proc_entry("glove_onoff", 0666, ctp_device_proc);
		   if (ctp_glove_onoff_proc == NULL) 
		   {
        MSG_DEBUG("ctp_gesture_onoff_proc create failed\n");
		   } 
		   else 
		   {
			   ctp_glove_onoff_proc->read_proc = proc_glove_onoff_read;
			   ctp_glove_onoff_proc->write_proc = proc_glove_onoff_write;
        MSG_DEBUG("ctp_glove_onoff_proc create success\n");
		   }
		   
	  
	  ///////////////uupdate   
		  ctp_version_proc = create_proc_entry("version", 0444, ctp_device_proc);
		  if (ctp_version_proc == NULL) 
		  {
        MSG_DEBUG("create_proc_entry version failed\n");
		  } 
		  else 
		  {
			  ctp_version_proc->read_proc = proc_version_gesture_read;
			  ctp_version_proc->write_proc = proc_version_gesture_write;
        MSG_DEBUG("create_proc_entry version success\n");
		  }
  
 }

#endif


