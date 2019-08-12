#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <hidapi/hidapi.h>

#define VENDOR_ID  0x22ea
#define PRODUCT_ID 0x001e

#define MAX_SIZE	64
#define IR_DATA_SIZE	 7
#define IR_DATA_SIZE_EX 35

#define BTO_CMD_ECHO_BACK		0x41
#define BTO_CMD_GET_DATA		0x50
#define BTO_CMD_SET_RECEIVE_MODE	0x51
#define BTO_CMD_GET_DATA_EX		0x52
#define BTO_CMD_SET_RECEIVE_MODE_EX	0x53
#define BTO_CMD_GET_FIRMWARE_VERSION	0x56
#define BTO_CMD_SET_DATA		0x60
#define BTO_CMD_SET_DATA_EX		0x61

#define RECEIVE_WAIT_MODE_NONE	0
#define RECEIVE_WAIT_MODE_WAIT	1

#define ECHO_BACK	0
#define GET_DATA	1
#define RECEIVE_MODE	2
#define GET_VERSION	3
#define SET_DATA	4

static char bto_commands[] = {
	BTO_CMD_ECHO_BACK,
	BTO_CMD_GET_DATA,
	BTO_CMD_SET_RECEIVE_MODE,
	BTO_CMD_GET_FIRMWARE_VERSION,
	BTO_CMD_SET_DATA
};
static char bto_commands_ex[] = {
	BTO_CMD_ECHO_BACK,
	BTO_CMD_GET_DATA_EX,
	BTO_CMD_SET_RECEIVE_MODE_EX,
	BTO_CMD_GET_FIRMWARE_VERSION,
	BTO_CMD_SET_DATA_EX
};

int varbose = 0;
void dump(char cType, const unsigned char* data,int size);

char get_command(int no, int extend) {
	char cmd;
	if (extend)
		cmd = bto_commands_ex[no];
	else
		cmd = bto_commands[no];
	return cmd;
}

int get_data_length(int extend) {
	if (extend)
		return IR_DATA_SIZE_EX;
	else
		return IR_DATA_SIZE;
}

int write_device(hid_device* device, const unsigned char *data, size_t length) {
	int ret;
	ret = hid_write(device, data, length);
	if(ret >= 0)
	{
		if(varbose != 0)
		{
			dump('S', data, ret);
		}
	}
	return ret;
}

int read_device(hid_device* device, unsigned char *data, size_t length, int mS) {
	int ret;

	ret = hid_read_timeout(device, data, length, mS);
	if(ret >= 0)
	{
		if(varbose != 0)
		{
			dump('R', data, ret);
		}
	}
	return ret;
}

int clear_device_buffer(hid_device* device) {
	unsigned char buf[MAX_SIZE + 1];
	int ret;

	memset(buf, 0x00, sizeof(buf));
	buf[0] = 0x00;
	buf[1] = BTO_CMD_ECHO_BACK;
	ret = write_device(device, buf, sizeof(buf));
	if(ret < 0)
	{
		fprintf(stderr,"ERROR: Can't wirte clear buffer.\n");
		return -1;
	}
	memset(buf,0xff,MAX_SIZE);
	ret = read_device(device, buf, MAX_SIZE, 500);
	if(ret < 0)
	{
		fprintf(stderr,"ERROR: Can't read clear buffer.\n");
		return -1;
	}
	return 0;
}

int receive_ir(hid_device* device, unsigned char *data, int length, int extend) {
	unsigned char buf[MAX_SIZE + 1];
	int i,ret;
	int retval = 0;
	unsigned char cmd;

	ret = clear_device_buffer(device);
	if(ret < 0)
	{
		return -1;
	}

	memset(buf, 0x00, sizeof(buf));
	buf[0] = 0x00;
	buf[1] = get_command(RECEIVE_MODE, extend);
	buf[2] = RECEIVE_WAIT_MODE_WAIT;
	ret = write_device(device, buf, sizeof(buf));
	if(ret < 0)
	{
		fprintf(stderr,"ERROR: Can't wirte wait mode.\n");
		return -1;
	}
	memset(buf, 0xff, MAX_SIZE);
	ret = read_device(device, buf, MAX_SIZE, 500);
	if(ret < 0)
	{
		fprintf(stderr,"ERROR: Can't read wait modes.\n");
		return -1;
	}

	sleep(3);

	cmd = get_command(GET_DATA, extend);
	memset(buf, 0x00, sizeof(buf));
	buf[0] = 0x00;
	buf[1] = get_command(GET_DATA, extend);
	ret = write_device(device, buf, sizeof(buf));
	if(ret < 0)
	{
		fprintf(stderr,"ERROR: Can't wirte get data.\n");
		return -1;
	}
	memset(buf, 0xff, MAX_SIZE);
	ret = read_device(device, buf, MAX_SIZE, 500);
	if(ret < 0)
	{
		fprintf(stderr,"ERROR: Can't read get data.\n");
		return -1;
	}
	if (buf[0] == cmd && buf[1] != 0) {
		for (i = 0; i < length; i++) {
			data[i] = buf[i+1];
		}
		retval = 1;
	}

	memset(buf, 0x00, sizeof(buf));
	buf[0] = 0x00;
	buf[1] = get_command(RECEIVE_MODE, extend);
	buf[2] = RECEIVE_WAIT_MODE_NONE;
	ret = write_device(device, buf, sizeof(buf));
	if(ret < 0)
	{
		fprintf(stderr,"ERROR: Can't write nowait mode.\n");
		return -1;
	}
	memset(buf, 0xff, MAX_SIZE);
	ret = read_device(device, buf, MAX_SIZE, 500);
	if(ret < 0)
	{
		fprintf(stderr,"ERROR: Can't read nowait mode.\n");
		return -1;
	}

	if(retval == 0)
	{
		fprintf(stderr,"Timeout.\n");
	}

	return retval;
}

int transfer_ir(hid_device* device, char *data, int length, int extend) {
	unsigned char buf[MAX_SIZE + 1];
	int i,ret;
	char *e, s[3] = "\0\0";

	memset(buf, 0x00, sizeof(buf));
	buf[0] = 0x00;
	buf[1] = get_command(SET_DATA, extend);

	// hex to byte
	for (i = 0; i < length; i++) {
		s[0] = data[i*2];
		s[1] = data[i*2+1];
		buf[2 + i] = (unsigned char)strtoul(s, &e, 16);
		if (*e != '\0') {
			break;
		}
	}

	if (i != length) {
		fprintf(stderr, "[%s] is skipped.\n", data);
		return -1;
	}

	ret = write_device(device, buf, sizeof(buf));
	if(ret < 0)
	{
		fprintf(stderr,"ERROR: Can't write set data.\n");
		return -1;
	}

	usleep(100 * 1000);

	ret = clear_device_buffer(device);
	if(ret < 0)
	{
		return -1;
	}
	return 0;
}

int close_device(hid_device* device) {
	if(device == NULL)
	{
		return -1;
	}
	hid_close(device);
	return 0;
}

hid_device* _open_device(int nDevNo)
{
	hid_device* device = NULL;
	int i = -1;

	struct hid_device_info *poi,*info = NULL;
	info = hid_enumerate(VENDOR_ID, PRODUCT_ID);
	if(info == NULL)
	{
		goto EXIT_PATH;
	}
	poi = info;
	while(poi)
	{
		if(poi->interface_number == 3)
		{
			i += 1;
			if(i == nDevNo)
			{
				break;
			}
		}
		poi = poi->next;
	}
	if(poi == NULL)
	{
		goto EXIT_PATH;
	}
	device = hid_open_path(poi->path);
EXIT_PATH:
	if(info != NULL)
	{
		hid_free_enumeration(info);
	}
	return device;
}

hid_device* open_device(int nDevNo) {
	int i;
	int ret;
	hid_device* device = NULL;
	for(i = 0;i < 100;i++)
	{
		device = _open_device(0);
		if(device == NULL)
		{
			continue;
		}
		ret = clear_device_buffer(device);
		if(ret < 0)
		{
			close_device(device);
			device = NULL;
			continue;
		}
		break;
	}
	return device;
}

void usage() {
	fprintf(stderr, "usage: bto_hid_cmd <option>\n");
	fprintf(stderr, "  -r		\tReceive Infrared code.\n");
	fprintf(stderr, "  -t <code>\tTransfer Infrared code.\n");
	fprintf(stderr, "  -e		\tExtend Infrared code.\n");
	fprintf(stderr, "			\t	Normal:	 7 octets\n");
	fprintf(stderr, "			\t	Extend: 35 octets\n");
	fprintf(stderr, "  -v		\tvarbose mode.\n");
}

int main(int argc, char *argv[]) {
	int ret = -1;
	int r;
	int i;
	unsigned char rbuf[IR_DATA_SIZE_EX+1];

	int receive_mode  = 0;
	int transfer_mode = 0;
	int extend = 0;
	int ir_data_size;
	char *ir_data = NULL;

	while ((r = getopt(argc, argv, "ehrt:v")) != -1) {
		switch(r) {
			case 'r':
				receive_mode = 1;
				break;
			case 't':
				transfer_mode = 1;
				ir_data = optarg;
				break;
			case 'e':
				extend = 1;
				fprintf(stderr,"Extend mode on.\n");
				break;
			case 'v':
				varbose = 1;
				break;
			default:
				usage();
				return -1;
		}
	}

	if (receive_mode == 1 || transfer_mode == 1) {
		/* hidapi initialize*/
		if ((r = hid_init()) < 0) {
			fprintf(stderr,"hid_init\n");
			return -1;
		}

		/* open device */
		hid_device *devh = open_device(0);
		if(!devh) {
			fprintf(stderr,"Can't open device.\n");
			return -1;
		}

		ir_data_size = get_data_length(extend);

		if (transfer_mode == 1) {
			printf("Transfer mode\n");
			printf("  Transfer code: %s\n", ir_data);
			ret = transfer_ir(devh, ir_data, ir_data_size, extend);
		}
		else if (receive_mode == 1) {
			fprintf(stderr,"Receive mode\n");
			fflush(stderr);
			memset(rbuf, 0xff, ir_data_size);
			r = receive_ir(devh, rbuf, ir_data_size, extend);
			if (r == 1) {
				fprintf(stderr,"  Received code: ");
				fflush(stderr);
				for (i = 0; i < ir_data_size; i++) {
					printf("%02X", rbuf[i]);
				}
				printf("\n");
				fflush(stdout);
				ret = 0;
			}
		}

		/* close device */
		close_device(devh);
	}
	else {
		usage();
		return -1;
	}

	return ret;
}

void dump(char cType, const unsigned char* data,int size)
{
	char c;
	int i,j;
	for(i = 0;i < size;i+=16)
	{
		fprintf(stderr,"%c %04X ",cType,i);
		for(j = 0;j < 16;j++)
		{
			if((i + j) >= size)
			{
				break;
			}
			c = data[i + j];
			fprintf(stderr,"%02X ",(int)c & 0xFF);
			if(j == 7)
			{
			 fprintf(stderr,"- ");
			}
		}
		for(/*j = 0*/;j < 16;j++)
		{
			fprintf(stderr,"   ");
			if(j == 7)
			{
				fprintf(stderr,"- ");
			}
		}
		for(j = 0;j < 16;j++)
		{
			if((i + j) >= size)
			{
				break;
			}
			c = data[i + j];
			fprintf(stderr,"%c",isprint(c) ? c : '?');
			if(j == 7)
			{
				fprintf(stderr," ");
			}
		}
		for(/*j = 0*/;j < 16;j++)
		{
			fprintf(stderr," ");
			if(j == 7)
			{
				fprintf(stderr," ");
			}
		}
		fprintf(stderr,"\n");
	}
	fflush(stderr);
}
