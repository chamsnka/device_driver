/*I2C DEVICE DRIVER*/

#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/device.h>
#include<linux/i2c.h>
#include<linux/delay.h>
#include<linux/uaccess.h>
#include<linux/slab.h>
#include<linux/kernel.h>
#include<linux/cdev.h>

#define LCD_SLAVE_ADDR 0x27
#define I2C_BUS_AVAILABLE 2
#define SLAVE_DEVICE_NAME "LCD_DISPLAY"


#define LCD_BACKLIGHT 0x08

#define LCD_CGRAM 0x40 //character Graphics ram 
#define LCD_DDRAM 0x80 //Display data ram

#define LCD_EN 0x04
#define LCD_RS 0x01


static dev_t dev =0;

/*structure declaration*/
static struct i2c_adapter *lcd_i2c_adapter=NULL;
static struct i2c_client *lcd_i2c_client=NULL;


static struct class *dev_class;

static int lcd_i2c_driver_probe(struct i2c_client *client, const struct i2c_device_id*id);
static int lcd_i2c_driver_remove(struct i2c_client*client);


/*
//LCD_WRITE FUNCTION//
static int I2C_write(unsigned char*databuf,unsigned int len)
{
	int ret;
	ret = i2c_master_send(lcd_i2c_client,databuf,len);
	
	return 0;
}

*/

static void lcd_send_command(unsigned char command)
{

	uint8_t databuf[4];

	uint8_t data_upper_nib = (command & 0xf0);
	uint8_t data_lower_nib= ((command<<4)& 0xf0);
	databuf[0] = (data_upper_nib| LCD_BACKLIGHT|LCD_EN);
	databuf[1] = (data_upper_nib| LCD_BACKLIGHT);
	databuf[2] = (data_lower_nib| LCD_BACKLIGHT|LCD_EN);
	databuf[3] = (data_lower_nib| LCD_BACKLIGHT);

//	ret= I2C_write(databuf,4);
	i2c_master_send(lcd_i2c_client ,databuf,4);
	printk(KERN_INFO"Lcd send command function....\n");
 
}



static void lcd_send_data(char data)
{
	uint8_t databuf[4];

	uint8_t data_upper_nib = (data & 0xf0);
	uint8_t data_lower_nib= ((data <<4)&0xf0);
	databuf[0] = data_upper_nib |LCD_BACKLIGHT |LCD_RS |LCD_EN;
	databuf[1] = data_upper_nib |LCD_BACKLIGHT |LCD_RS;
	
	databuf[2] = data_lower_nib |LCD_BACKLIGHT |LCD_EN |LCD_RS;
	databuf[3] = data_lower_nib |LCD_BACKLIGHT |LCD_RS;

//	ret= I2C_write(databuf,4);
	i2c_master_send(lcd_i2c_client,databuf,4);

	printk(KERN_INFO"Lcd send data function.....\n");
}



static int lcd_init(void)
{

	/* LCD DISPLAY INITIALIZATION*/

	/* 4Bit Initialization */

	msleep(25);	// wait for ->15ms
	
	lcd_send_command(0x03);
	
	msleep(5);     


	/*display initialization*/

	lcd_send_command(0x20);  //LCD_FUNCTION_SET, LCD_FUNCTION_2LINE,LCD_FUNCTION_DL_4BIT 

	msleep(5);
	printk(KERN_INFO"the function set done\n");

	lcd_send_command(0x08); //LCD_DISPLAY_OFF //DISPLAY OFF CONTROL
	
	msleep(5);

	printk(KERN_INFO"the display off done\n");
	

	lcd_send_command(0x01); //LCD_CLEAR_DISPLAY; // CLEAR THE DISPLAY
	msleep(5);
        printk(KERN_INFO"the display clear done\n");

	lcd_send_command(0x06); ///LCD_ENTRY_MODE_SET);//Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift)
	msleep(5);
	printk(KERN_INFO"the entry mode done\n");

	lcd_send_command(0x0C); //LCD_DISPLAY_ON);//Display on control --> D = 1, C and B = 0. (Cursor and blink, last two bits)
	msleep(5);
	printk(KERN_INFO"the display on done\n");

	return 0;


}


static void lcd_send_string(char *str,int len)
{
	int i;
	for(i=0;i<len-1;i++)
	{
		lcd_send_data(str[i]);
		printk(KERN_INFO"The string is %c\n",str[i]);

		
	}
	printk(KERN_INFO"LCD string... \n");

}


/*structure that has the slave device id*/

static const struct i2c_device_id lcd_i2c_driver_id[] ={
	{ SLAVE_DEVICE_NAME,0},
	{}
};

MODULE_DEVICE_TABLE(i2c ,lcd_i2c_driver_id);

/*I2C driver Structure that has to be added to linux*/

static struct i2c_driver lcd_i2c_driver = {
	.driver={
		.name = SLAVE_DEVICE_NAME,
		.owner= THIS_MODULE,
	},
	.probe    = lcd_i2c_driver_probe,
	.remove   = lcd_i2c_driver_remove,
	.id_table = lcd_i2c_driver_id,
};



/*I2c board info structre*/

static struct i2c_board_info lcd_i2c_board_info =
{
	I2C_BOARD_INFO(SLAVE_DEVICE_NAME ,LCD_SLAVE_ADDR)
};

/*This function getting called when the slave has been found
  This will be called only once when we load the driver.*/

static int lcd_i2c_driver_probe(struct i2c_client *client ,const struct i2c_device_id *id)
{
	int len;
	char str[]="CHANDAR MOULI K";
	printk(KERN_INFO"i2c_probe() invoked %d:%s:%s\n",__LINE__,__func__,__FILE__);
	lcd_init();

	printk(KERN_INFO"LCD INIT IS DONE SUCCESFULLY\n");
	len=sizeof(str);
	printk(KERN_INFO"The string size %d\n",len);
	printk(KERN_INFO"The string name %s\n",str);

	lcd_send_string(str,len);

	return 0;

}


/*This function getting called when the slave has been removed 
  This will be called only once when we unload the driver.
  */

static int lcd_i2c_driver_remove(struct i2c_client *client)
{
	printk(KERN_INFO"i2c_removed () successfull.. %d:%s:%s\n",__LINE__,__func__,__FILE__);
	return 0;
}


/*initialization*/

static int __init i2c_driver_init(void)
{
	int ret =-1;
	
	printk(KERN_INFO"%d:%s:%s\n",__LINE__,__func__,__FILE__);

	/*Allocating major number*/
	
	if((alloc_chrdev_region(&dev,0,1,"i2c_client_driver"))<0)

	{
		printk(KERN_INFO "%s :allocate chardev_region failed..\n",__FILE__);
		return -1;
	}
	printk(KERN_INFO"major =%d minor =%d..\n",MAJOR(dev),MINOR(dev));

	/*creating struct class*/
	if((dev_class = class_create(THIS_MODULE,"i2c_client_class"))==NULL)
	{
		printk(KERN_INFO "cannot create the struct class..\n");

	}
	/*creating device */

	if((device_create(dev_class,NULL,dev,NULL,"i2c_client_dev")) ==NULL)
	{
		printk(KERN_INFO "cannot create the device..\n");
	}
	/*i2c driver add*/
	lcd_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
	
	printk(KERN_INFO"%s : i2c_get_adapter() Successfull!\n",__FILE__);

	if(lcd_i2c_adapter != NULL)

	{
		lcd_i2c_client=i2c_new_client_device(lcd_i2c_adapter,&lcd_i2c_board_info);
		
		printk(KERN_INFO"%s : i2c_new_device() Successfull!\n",__FILE__);
		
		if(lcd_i2c_client!=NULL)
		{
			/*the function i2c_add_driver should be called with a pointer to the struct i2c_driver*/
			i2c_add_driver(&lcd_i2c_driver);
			
			printk(KERN_INFO"%s : i2c_add_driver_Successfull!\n",__FILE__);

			ret =0;

		}

		i2c_put_adapter(lcd_i2c_adapter);
		
		printk(KERN_INFO"%s : i2c_put_adapter_Successfull!\n",__FILE__);

	}
	printk(KERN_INFO"Driver Added\n");

	return ret;

}



static void __exit i2c_driver_exit(void)
{
	printk(KERN_INFO"%d:%s:%s\n",__LINE__,__func__,__FILE__);
	i2c_del_driver(&lcd_i2c_driver);
	printk(KERN_INFO"%s : i2c_del_driver() invoked !\n",__FILE__);
	i2c_unregister_device(lcd_i2c_client);
	device_destroy(dev_class,dev);
	class_destroy(dev_class);
	printk(KERN_INFO"%s : class_destroy() destroy dev class !\n",__FILE__);
	unregister_chrdev_region(dev,1);
	printk(KERN_INFO"%s : unregister_chardev_reg() released dev num !\n",__FILE__);
	printk(KERN_INFO "Driver removed\n");
}


module_init(i2c_driver_init);
module_exit(i2c_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("i2c client driver");
MODULE_AUTHOR("chandarmouli");

