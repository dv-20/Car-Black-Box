#include <xc.h>
#include "main.h"
#include "clcd.h"
#include "ds1307.h"
#include "i2c.h"
#include"matrix_keypad.h"
#include "adc.h"
#include<string.h>
#include "uart.h"
#include "eeprom.h"
unsigned char clock_reg[3];
unsigned char calender_reg[4];
unsigned char time[9];
unsigned char date[11];

void display_date(void)
{
	clcd_print(date, LINE2(3));
}

void display_time(void)
{
	clcd_print(time, LINE2(0));

	if (clock_reg[0] & 0x40)
	{
		if (clock_reg[0] & 0x20)
		{
			clcd_print("PM", LINE2(12));
		}
		else
		{
			clcd_print("AM", LINE2(12));
		}
	}
}

static void get_time(void)
{
	clock_reg[0] = read_ds1307(HOUR_ADDR);
	clock_reg[1] = read_ds1307(MIN_ADDR);
	clock_reg[2] = read_ds1307(SEC_ADDR);

	if (clock_reg[0] & 0x40)
	{
		time[0] = '0' + ((clock_reg[0] >> 4) & 0x01);
		time[1] = '0' + (clock_reg[0] & 0x0F);
	}
	else
	{
		time[0] = '0' + ((clock_reg[0] >> 4) & 0x03);
		time[1] = '0' + (clock_reg[0] & 0x0F);
	}
	time[2] = ':';
	time[3] = '0' + ((clock_reg[1] >> 4) & 0x0F);
	time[4] = '0' + (clock_reg[1] & 0x0F);
	time[5] = ':';
	time[6] = '0' + ((clock_reg[2] >> 4) & 0x0F);
	time[7] = '0' + (clock_reg[2] & 0x0F);
	time[8] = '\0';
}


void init_config(void)
{
	init_clcd();
	init_i2c();
	init_ds1307();
	init_uart();
	init_adc();
	init_matrix_keypad();

}
static unsigned char gear_pos;
unsigned char get_gear_pos() {


	char key = read_switches(STATE_CHANGE);

	if (key == GEAR_UP) {
		if (gear_pos < MAX_GEAR)
			gear_pos++;
		else
			gear_pos = 7;

	} else if (key == GEAR_DOWN) {
		if (gear_pos > 1)
			gear_pos--;
		else
			gear_pos = 0;
	}
	else if (key == COLLISION) {
		gear_pos = 8;
	}
	if (gear_pos == 0)
		return 'N';
	else if (gear_pos == 7)
		return 'R';
	else if(gear_pos==8)
		return 'C';
	else
		return gear_pos + '0';

}
uint16_t get_speed() {
	uint16_t adc_reg_val = read_adc(SPEED_ADC_CHANNEL);
	int speed;
	speed=(adc_reg_val / 10.33);
	switch(gear_pos)
	{
	case 0:
		return 0;
	case 1:
		return (float)speed*0.202>10?(float)speed*0.202:10;
	case 2:
		return (float)speed*0.404>20?(float)speed*0.404:20;
	case 3:
		return (float)speed*0.606>30?(float)speed*0.606:30;
	case 4:
		return (float)speed*0.757>40?(float)speed*0.757:40;
	case 5:
		return (float)speed*0.858>50?(float)speed*0.858:50;
	case 6:
        return (float)speed>60?(float)speed:60;
    case 7:
        return 20;
    case 8:
        return 0;
            
	}


}
void reverse(char *str) {
	int i = 0;
	char temp;
	int len = strlen(str);
	while (i < len / 2) {
		temp = str[i];
		str[i] = str[len - 1 - i];
		str[len - 1 - i] = temp;
		i++;
	}
}

void ITOA(unsigned short data, char *str) {
	int i = 0;
	char temp;
	if (data < 10) {
		str[0] = data + '0';
		str[1] = 0;
		return;
	}
	while (data) {
		temp = data % 10;
		data = data / 10;
		str[i++] = temp + '0';
	}
	str[i] = '\0';
	reverse(str);

}

unsigned char address=0;
void store(char *time,char *speed,char gear)
{
	if(address<252)
	{
		for(int i=0; i<8; i++)
		{
			if(i==2||i==5)
			{
				continue;
			}
			write_internal_eeprom(address++,time[i]);
		}
		write_internal_eeprom(address++,gear);
		for(int i=0; i<2; i++)
		{
			write_internal_eeprom(address++,speed[i]);
		}
	}
	else
	{
		for(int j=0; j<=27; j++)
		{
			for(int i=0; i<9; i++)
			{
				write_internal_eeprom((j*9)+i,read_internal_eeprom((j+1)*9 + i));
			}
		}
		address=address-9;
		for(int i=0; i<8; i++)
		{
			if(i==2||i==5)
			{
				continue;
			}
			write_internal_eeprom(address++,time[i]);
		}
		write_internal_eeprom(address++,gear);
		for(int i=0; i<2; i++)
		{
			write_internal_eeprom(address++,speed[i]);
		}
		//store(time,speed,gear);
	}
	//clcd_putch((address/9)+'0',LINE1(15));
	return;
}
void display_menu(char pos,char flag)
{
	char print[4][16]= {"VIEW LOGS   ","DOWNLOAD LOGS ","CLEAR LOGS   ","SET TIME   "};

	if(flag==0)
	{
        CLEAR_DISP_SCREEN;
		clcd_print("->",LINE1(0));
		clcd_print(print[pos],LINE1(2));
		clcd_print(print[pos+1],LINE2(0));
	}
	else
	{
        CLEAR_DISP_SCREEN;
		clcd_print("->",LINE2(0));
		clcd_print(print[pos-1],LINE1(0));
		clcd_print(print[pos],LINE2(2));
	}


}
void read(char *data1,int i)
{
	int address=0;
	char data[12]= {0};
	for(int j=0; j<11; j++)
	{
		if(j==2 || j==5)
		{
			data[j]=':';
		}
		else
			data[j]=read_internal_eeprom((address++)+(i*9));
	}
	int j=0;
	for(int i=0; i<14; i++)
	{
		if(i==8 || i== 10)
		{
			data1[i]=' ';
		}
		else
			data1[i]=data[j++];

	}
}
void download()
{
	clcd_print("   DOWNLOAD   ",LINE1(0));
	clcd_putch(address/9+'0',LINE1(15));
	char data[14]= {0};
	puts(" TIME   GR SP\n\r");
	//puts("\n\r");
	for(int i=0; i<address/9; i++)
	{
		read(data,i);
		//clcd_print(data,LINE2(0));
		puts(data);
		puts("\n\r");
	}
	return ;

}
void view_log()
{
	int i=0;
	clcd_print("VL TIME GR SP ",LINE1(0));
    clcd_putch((address/9)/10+'0',LINE1(14));
    clcd_putch((address/9)%10+'0',LINE1(15));
	while(1)
	{
         
		char data[14]= {0};
		char key=read_switches(STATE_CHANGE);
		if(key==MK_SW1 && i>0)
		{
			i--;
		}
		else if(key==MK_SW2 && i <(address/9)-1)
		{
			i++;
		}
		else if(key==MK_SW5)
		{
			return ;
		}
        if(address==0)
        {
          clcd_print("    NO  LOGS   ",LINE2(0));
           
        }
        else
        {
		  read(data,i);
		  clcd_print(data,LINE2(0));
        }
	}
}
void set_time()
{
	clcd_print("   SET TIME  ",LINE1(0));
	char flag=0;
	char key;
	signed char hr=((time[0]-'0')*10)+time[1]-'0';
	signed char min=(time[3]-'0')*10+time[4]-'0';
	signed char sec=(time[6]-'0')*10+time[7]-'0';
    int delay=0;
	while(1)
	{
		key=read_switches(STATE_CHANGE);
		if(key==MK_SW1)
		{
			if(flag==2)
			{
				sec++;
				if(sec==60)
					sec=0;
                time[6]=sec/10+'0';
                time[7]=sec%10+'0';
                
			}
			else if(flag==1)
			{
				min++;
				if(min==60)
					min=0;
                time[3]=min/10+'0';
                time[4]=min%10+'0';
			}
			else if(flag==0)
			{
				hr++;
				if(hr==24)
				{
					hr=0;
				}
                time[0]=hr/10+'0';
                time[1]=hr%10+'0';
			}
		}
		else if(key==MK_SW2)
		{
			if(flag==2)
			{
				sec--;
				if(sec<0)
					sec=59;
                time[6]=sec/10+'0';
                time[7]=sec%10+'0';
			}
			else if(flag==1)
			{
				min--;
				if(min<0)
					min=59;
                time[3]=min/10+'0';
                time[4]=min%10+'0';
			}
			else if(flag==0)
			{
				hr--;
				if(hr<0)
				{
					hr=23;
				}
                time[0]=hr/10+'0';
                time[1]=hr%10+'0';
			}

		}
		else if(key==MK_SW3)
		{
			flag++;
			if(flag==3)
				flag=0;

		}
		else if(key==MK_SW4)
        {
			write_ds1307(SEC_ADDR,0x80);
		//write_ds1307(SEC_ADDR,sec%10);
		write_ds1307(SEC_ADDR,((sec/10)<<4)|(sec%10));
		// write_ds1307(MIN_ADDR,min%10);
		write_ds1307(MIN_ADDR,((min/10)<<4)|(min%10));
		//write_ds1307(HOUR_ADDR,hr%10);
		write_ds1307(HOUR_ADDR,(((hr/10)<<4)&0xBF)|(hr%10));
		  char dummy=read_ds1307(SEC_ADDR);
		  write_ds1307(SEC_ADDR,dummy&(~(0x80)));
		return;
        }
        else if(key==MK_SW5)
        {
            break;
        }
        if(delay++<200)
        clcd_print(time,LINE2(0));
        else if(delay>200 && delay<400)
            clcd_print("  ",LINE2(3*flag));
        else
            delay=0;
        
	}

}
void main(void)
{
	init_config();
	unsigned int speed;
	char speed_arr[]="00";
	clcd_print("TIME     GR SPD",LINE1(0));
	char mode=0;
	char gear=0;
	char prev_gear=0;
	char pos=0;
	char switch_flag=1;
	char key;
	char line_flag=0;
	while (1)
	{

		if(switch_flag)
		{
			if(read_switches(STATE_CHANGE)==MODE_MENU)
			{

				mode=1;
				pos=0;
                line_flag=0;
				CLEAR_DISP_SCREEN;
                display_menu(pos,line_flag);
			}
			if(mode==0)
			{
				clcd_print("TIME     GR SPD",LINE1(0));
				get_time();
				display_time();
				gear=get_gear_pos();
				clcd_putch(gear,LINE2(10));
				speed=get_speed();
				ITOA(speed,speed_arr);
				clcd_print(speed_arr,LINE2(13));
				if(prev_gear!=gear)
				{
					prev_gear=gear;
					store(time,speed_arr,gear);
				}
				if(read_switches(STATE_CHANGE)==MODE_MENU)
				{

					mode=1;
					pos=0;
                    line_flag=0;
					CLEAR_DISP_SCREEN;
                    display_menu(pos,line_flag);
				}

			}
			else
			{

				key=read_switches(STATE_CHANGE);
//              if(pre!=0xFF)
//              {
//                  key=pre;
//              }
				if(key!=0xFF)
				{
					if(key==MK_SW1 && pos>0)
					{
						if(line_flag==1)
						{
							line_flag=0;
						}
						pos--;
					}
					else if(key==MK_SW2 && pos <3)
					{
						if(line_flag==0)
							line_flag=1;
						pos++;
					}
					else if(key==MK_SW3)
					{
						switch_flag=0;
					}
					else if(key==MK_SW5)
					{

						mode=0;
						CLEAR_DISP_SCREEN;
					}
					if(mode)
						display_menu(pos,line_flag);
				}
				// pre=read_switches(STATE_CHANGE);
			}
		}
		else
		{
			clcd_putch(pos+'0',LINE1(15));
			switch(pos)
			{
			case 0:
				CLEAR_DISP_SCREEN;
				view_log();
				CLEAR_DISP_SCREEN;
				break;
			case 1:
				CLEAR_DISP_SCREEN;
				download();
				CLEAR_DISP_SCREEN;
				break;
			case 2:
				address=0;
                CLEAR_DISP_SCREEN;
				break;
			case 3:
				CLEAR_DISP_SCREEN;
				set_time();
				CLEAR_DISP_SCREEN;
				break;
			}
			switch_flag=1;
			mode=1;
			line_flag=0;
			pos=0;
            display_menu(pos,line_flag);
//            key=read_switches(STATE_CHANGE);
//            if(key==MK_SW5)
//            {
//                switch_flag=1;
//            }
		}
	}
}