#include<stdio.h>
#include<sys/io.h>
#include<unistd.h>
#include<math.h>

#define REG		0x2e
#define VAL		0x2f
#define CHIPID		0x20
#define CHIPREV		0x22

static int superio_inw(int reg)
{
	int val;

	outb(reg++, REG);
	val = inb(VAL) << 8;
	outb(reg, REG);
	val |= inb(VAL);
	return val;
}

static int superio_inb(int reg)
{
	outb(reg, REG);
	return inb(VAL);
}

static void init_ec()
{
	//Enter MB PnP Mode
	outb(0x87, REG);
	outb(0x01, REG);
	outb(0x55, REG);
	outb(0x55, REG);

	unsigned short chip_type = superio_inw(CHIPID);
	unsigned char chip_rev  = superio_inb(CHIPREV) & 0x0f;
	printf("Found Chip IT%04x rev %x.\n", chip_type, chip_rev);

	//Logical Device Number (LDN, Index=07h)
	outb(0x07, REG);
	//Environment Controller Configuration Registers (LDN=04h)
	outb(0x04, VAL);
	//LDN(Logical Device) Active
	outb(0x30,REG);
	outb(0x01,VAL);
}

int main()
{
    unsigned int ec_base,addr_port,data_port,fan_speed,fan_rpm;

	int ret = iopl(3);
	if(ret == -1)
	{
		printf("iopl error.\n");
		return -1;
	}
	
	init_ec();

	//Read EC Base Address
	//(LDN=04h, registers index=60h, 61h).
	ec_base = superio_inw(0x60);
	printf("ec_base addr:0x%x\n", ec_base);
	//Address register of EC
	addr_port = ec_base+0x05;
	//Data register of EC
	data_port = ec_base+0x06;
	printf("addr_port is 0x%x\n",addr_port);
	printf("data_port is 0x%x\n",data_port);

	//Fan Tachometer Control Register
	outb(0x0c, addr_port);
	outb(0x00, data_port);

	//1:CPU_FAN 2:SYS_FAN 3:PWR_FAN
	//1-3 Reading Registers[7:0] (Index=0Dh-0Fh)
	outb(0x0d,addr_port);
	int lval = inb(data_port);
	//1-3 Extended Reading Registers [15:8] (Index=18h-1Ah)
	outb(0x18,addr_port);
	int mval = inb(data_port);
	fan_speed = (mval<<8) | lval;
	printf("mval:%02x lval:%02x fan_speed is %d\n",mval, lval, fan_speed);

	//fan_rpm = 1.35 * 10^6 / (Count * Divisor);(Default Divisor = 2)
	fan_rpm = 1.35*pow(10,6)/(fan_speed*2);
	printf("fan_rpm is %d\n",fan_rpm);

	/*
	//enable tachometer,set SmartGuardian mod(pwm:256)
	outb(0x13, addr_port);
	outb(0x77, data_port);
	//set PWM values: active high polarity, 23.43 kHz Base clock, 20% minimum duty,
	outb(0x14, addr_port);
	outb(0xc0, data_port);//default:40
	//set fan full speed
	outb(0x15, addr_port);
	outb(0x7f, data_port);//default:00
	*/
	outb(0x29, addr_port);
	float tmpin1 = inb(data_port);
	printf("tmpin1 is %.1f °C\n",tmpin1);

	//Exit MB PnP Mode
	outb(0x02, REG);
	outb(0x02, VAL);
	iopl(0);
	return 0;
}