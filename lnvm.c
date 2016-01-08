#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <argp.h>
#include "linux/lightnvm.h"

enum cmdtypes {
	LIGHTNVM_INFO = 1,
	LIGHTNVM_DEVS,
	LIGHTNVM_CREATE,
	LIGHTNVM_REMOVE,
	LIGHTNVM_DEV_INIT,
	LIGHTNVM_DEV_FACTORY,
};

struct arguments
{
	int cmdtype;
	int arg_num; /* state->arg_num doesn't increase with subcommands */
	char *tgtname;
	char *tgttype;
	char *devname;
	char *mmtype;

	int lun_begin;
	int lun_end;

	int erase_only;
	int hostmarks;
	int bbmarks;
};

static int list_info(int fd)
{
	struct nvm_ioctl_info c;
	int ret, i;

	memset(&c, 0, sizeof(struct nvm_ioctl_info));

	ret = ioctl(fd, NVM_INFO, &c);
	if (ret)
		return ret;

	printf("LightNVM (%u,%u,%u). %u target type(s) registered.\n",
			c.version[0], c.version[1], c.version[2], c.tgtsize);
	printf("Type\tVersion\n");

	for (i = 0; i < c.tgtsize; i++) {
		struct nvm_ioctl_info_tgt *tgt = &c.tgts[i];

		printf("%s\t(%u,%u,%u)\n",
				tgt->tgtname, tgt->version[0], tgt->version[1],
				tgt->version[2]);
	}

	return 0;
}

static int list_devices(int fd)
{
	struct nvm_ioctl_get_devices devs;
	int ret, i;

	ret = ioctl(fd, NVM_GET_DEVICES, &devs);
	if (ret)
		return ret;

	printf("Number of devices: %u\n", devs.nr_devices);
	printf("%-12s\t%-12s\tVersion\n", "Device", "Block manager");

	for (i = 0; i < devs.nr_devices && i < 31; i++) {
		struct nvm_ioctl_device_info *info = &devs.info[i];

		printf("%-12s\t%-12s\t(%u,%u,%u)\n", info->devname, info->bmname,
				info->bmversion[0], info->bmversion[1],
				info->bmversion[2]);
	}

	return 0;
}

static int create_tgt(int fd, struct arguments *args)
{
	struct nvm_ioctl_create c;
	int ret;

	strncpy(c.dev, args->devname, DISK_NAME_LEN);
	strncpy(c.tgtname, args->tgtname, DISK_NAME_LEN);
	strncpy(c.tgttype, args->tgttype, NVM_TTYPE_NAME_MAX);
	c.flags = 0;
	c.conf.type = 0;
	c.conf.s.lun_begin = args->lun_begin;
	c.conf.s.lun_end = args->lun_end;

	ret = ioctl(fd, NVM_DEV_CREATE, &c);
	if (ret)
		fprintf(stderr, "create failed. See dmesg.\n");

	return ret;
}

static int remove_tgt(int fd, struct arguments *args)
{
	struct nvm_ioctl_remove c;
	int ret;

	strncpy(c.tgtname, args->tgtname, DISK_NAME_LEN);
	c.flags = 0;

	ret = ioctl(fd, NVM_DEV_REMOVE, &c);
	if (ret)
		fprintf(stderr, "remove failed. See dmesg.\n");

	return ret;
}

static int dev_init(int fd, struct arguments *args)
{
	struct nvm_ioctl_dev_init init;
	int ret;

	memset(&init, 0, sizeof(struct nvm_ioctl_dev_init));

	strncpy(init.dev, args->devname, DISK_NAME_LEN);
	if (args->mmtype)
		strncpy(init.mmtype, args->mmtype, NVM_MMTYPE_LEN);
	else
		strncpy(init.mmtype, "gennvm", NVM_MMTYPE_LEN);

	ret = ioctl(fd, NVM_DEV_INIT, &init);
	switch (errno) {
	case EINVAL:
		printf("Initialization failed\n");
		break;
	case EEXIST:
		printf("Device has already been initialized.\n");
		break;
	case 0:
		break;
	default:
		printf("Unknown error occurred (%d)\n", errno);
		break;
	}

	return ret;
}

static int dev_factory(int fd, struct arguments *args)
{
	struct nvm_ioctl_dev_factory fact;
	int ret;

	memset(&fact, 0, sizeof(struct nvm_ioctl_dev_factory));

	strncpy(fact.dev, args->devname, DISK_NAME_LEN);
	if (args->erase_only)
		fact.flags |= NVM_FACTORY_ERASE_ONLY_USER;
	if (args->hostmarks)
		fact.flags |= NVM_FACTORY_RESET_HOST_BLKS;
	if (args->bbmarks)
		fact.flags |= NVM_FACTORY_RESET_GRWN_BBLKS;

	ret = ioctl(fd, NVM_DEV_FACTORY, &fact);
	switch (errno) {
	case EINVAL:
		printf("Factory reset failed.\n");
		break;
	case 0:
		break;
	default:
		printf("Unknown error occurred (%d)\n", errno);
		break;
	}

	return ret;
}


static struct argp_option opt_create[] =
{
	{"device", 'd', "DEVICE", 0, "LightNVM device e.g. nvme0n1"},
	{"tgtname", 'n', "TGTNAME", 0, "Target name e.g. tgt0"},
	{"tgttype", 't', "TGTTYPE", 0, "Target type e.g. rrpc"},
	{"tgtoptions", 'o', "", 0, "Options e.g. 0:0"},
	{0}
};

static char doc_create[] =
		"\n\vExamples:\n"
		" Init target (tgt0) with (nvme0n1) device using rrpc on lun 0.\n"
		"  lnvm create -d nvme0n1 -n tgt0 -t rrpc\n"
		" Init target (test0) with (nulln0) device using rrpc on luns [0->3].\n"
		"  lnvm create -d nulln0 -n test0 -t rrpc -o 0:3\n";

static error_t parse_create_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *args = state->input;
	int vars;

	switch (key) {
	case 'd':
		if (!arg || args->devname)
			argp_usage(state);
		if (strlen(arg) > DISK_NAME_LEN) {
			printf("Argument too long\n");
			argp_usage(state);
		}
		args->devname = arg;
		args->arg_num++;
		break;
	case 'n':
		if (!arg || args->tgtname)
			argp_usage(state);
		if (strlen(arg) > DISK_NAME_LEN) {
			printf("Argument too long\n");
			argp_usage(state);
		}
		args->tgtname = arg;
		args->arg_num++;
		break;
	case 't':
		if (!arg || args->tgttype)
			argp_usage(state);
		if (strlen(arg) > NVM_TTYPE_MAX) {
			printf("Argument too long\n");
			argp_usage(state);
		}
		args->tgttype = arg;
		args->arg_num++;
		break;
	case 'o':
		if (!arg)
			argp_usage(state);
		vars = sscanf(arg, "%u:%u", &args->lun_begin, &args->lun_end);
		if (vars != 2)
			argp_usage(state);
		break;
	case ARGP_KEY_ARG:
		if (args->arg_num > 4)
			argp_usage(state);
		break;
	case ARGP_KEY_END:
		if (args->arg_num < 3)
			argp_usage(state);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp_create = { opt_create, parse_create_opt, 0, doc_create};

static void cmd_create(struct argp_state *state, struct arguments *args)
{
	int argc = state->argc - state->next + 1;
	char** argv = &state->argv[state->next - 1];
	char* argv0 = argv[0];

	argv[0] = malloc(strlen(state->name) + strlen(" create") + 1);
	if(!argv[0])
		argp_failure(state, 1, ENOMEM, 0);

	sprintf(argv[0], "%s create", state->name);

	argp_parse(&argp_create, argc, argv, ARGP_IN_ORDER, &argc, args);

	free(argv[0]);
	argv[0] = argv0;
	state->next += argc - 1;
}

static struct argp_option opt_remove[] =
{
	{"tgtname", 'n', "TGTNAME", 0, "Target name e.g. tgt0"},
	{0}
};

static char doc_remove[] =
		"\n\vExample:\n"
		" Remove target (tgt0).\n"
		"  lnvm remove tgt0\n"
		"  lnvm remove -n tgt0\n";

static error_t parse_remove_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *args = state->input;

	switch (key) {
	case 'n':
		if (!arg || args->tgtname)
			argp_usage(state);
		if (strlen(arg) > DISK_NAME_LEN) {
			printf("Argument too long\n");
			argp_usage(state);
		}
		args->tgtname = arg;
		args->arg_num++;
		break;
	case ARGP_KEY_ARG:
		if (args->arg_num > 1)
			argp_usage(state);
		if (arg) {
			args->tgtname = arg;
			args->arg_num++;
		}
		break;
	case ARGP_KEY_END:
		if (args->arg_num < 1)
			argp_usage(state);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp_remove = {opt_remove, parse_remove_opt,
					"[TGTNAME|-n TGTNAME]", doc_remove};

static void cmd_remove(struct argp_state *state, struct arguments *args)
{
	int argc = state->argc - state->next + 1;
	char** argv = &state->argv[state->next - 1];
	char* argv0 = argv[0];

	argv[0] = malloc(strlen(state->name) + strlen(" remove") + 1);
	if(!argv[0])
		argp_failure(state, 1, ENOMEM, 0);

	sprintf(argv[0], "%s remove", state->name);

	argp_parse(&argp_remove, argc, argv, ARGP_IN_ORDER, &argc, args);

	free(argv[0]);
	argv[0] = argv0;
	state->next += argc - 1;
}

static struct argp_option opt_dev_init[] =
{
	{"device", 'd', "DEVICE", 0, "LightNVM device e.g. nvme0n1"},
	{"mmname", 'm', "MMNAME", 0, "Media Manager. Default: gennvm"},
	{0}
};

static char doc_dev_init[] =
		"\n\vExamples:\n"
		" Init nvme0n1 device\n"
		"  lnvm init -d nvme0n1\n"
		" Init nvme0n1 device with other media manager\n"
		"  lnvm init -d nvne0n1 -m other\n";

static error_t parse_dev_init_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *args = state->input;

	switch (key) {
	case 'd':
		if (!arg || args->devname)
			argp_usage(state);
		if (strlen(arg) > DISK_NAME_LEN) {
			printf("Argument too long\n");
			argp_usage(state);
		}
		args->devname = arg;
		args->arg_num++;
		break;
	case 'm':
		if (!arg || args->mmtype)
			argp_usage(state);
		if (strlen(arg) > NVM_MMTYPE_LEN) {
			printf("Argument too long\n");
			argp_usage(state);
		}
		args->mmtype = arg;
		args->arg_num++;
		break;
	case ARGP_KEY_ARG:
		if (args->arg_num > 2)
			argp_usage(state);
		break;
	case ARGP_KEY_END:
		if (args->arg_num < 1)
			argp_usage(state);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp_dev_init = {opt_dev_init, parse_dev_init_opt,
							0, doc_dev_init};

static void cmd_dev_init(struct argp_state *state, struct arguments *args)
{
	int argc = state->argc - state->next + 1;
	char** argv = &state->argv[state->next - 1];
	char* argv0 = argv[0];

	argv[0] = malloc(strlen(state->name) + strlen(" init") + 1);
	if(!argv[0])
		argp_failure(state, 1, ENOMEM, 0);

	sprintf(argv[0], "%s init", state->name);

	argp_parse(&argp_dev_init, argc, argv, ARGP_IN_ORDER, &argc, args);

	free(argv[0]);
	argv[0] = argv0;
	state->next += argc - 1;
}

static struct argp_option opt_dev_factory[] =
{
	{"device", 'd', "DEVICE", 0, "LightNVM device e.g. nvme0n1"},
	{"erase_only_marked", 'e', 0, 0, "Only erase marked blocks. Default: all blocks"},
	{"hostmarks", 'h', 0, 0, "Mark host-side blocks free. Default: keep"},
	{"bbmarks", 'b', 0, 0, "Mark grown bad blocks free. Default: keep"},
	{0}
};

static char doc_dev_factory[] =
		"\n\vExamples:\n"
		" Erase disk, but keep LightNVM disk format\n"
		"  lnvm factory -d nvme0n1\n"
		" Erase disk\n"
		"  lnvm factory -d nvme0n1 -h\n"
		" Erase disk and reset grown bad list as well\n"
		"  lnvm factory -d nvme0n1 -h -b\n"
		" Only erase LightNVM disk format related blocks\n"
		"  lnvm factory -d nvme0n1 -h -e\n";


static error_t parse_dev_factory_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *args = state->input;

	switch (key) {
	case 'd':
		if (!arg || args->devname)
			argp_usage(state);
		if (strlen(arg) > DISK_NAME_LEN) {
			printf("Argument too long\n");
			argp_usage(state);
		}
		args->devname = arg;
		args->arg_num++;
		break;
	case 'e':
		if (args->erase_only)
			argp_usage(state);
		args->erase_only = 1;
		args->arg_num++;
		break;
	case 'h':
		if (args->hostmarks)
			argp_usage(state);
		args->hostmarks = 1;
		args->arg_num++;
		break;
	case 'b':
		if (args->bbmarks)
			argp_usage(state);
		args->bbmarks = 1;
		args->arg_num++;
		break;
	case ARGP_KEY_ARG:
		if (args->arg_num > 4)
			argp_usage(state);
		break;
	case ARGP_KEY_END:
		if (args->arg_num < 1)
			argp_usage(state);
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp_dev_factory = {opt_dev_factory, parse_dev_factory_opt,
							0, doc_dev_factory};

static void cmd_dev_factory(struct argp_state *state, struct arguments *args)
{
	int argc = state->argc - state->next + 1;
	char** argv = &state->argv[state->next - 1];
	char* argv0 = argv[0];

	argv[0] = malloc(strlen(state->name) + strlen(" factory") + 1);
	if(!argv[0])
		argp_failure(state, 1, ENOMEM, 0);

	sprintf(argv[0], "%s factory", state->name);

	argp_parse(&argp_dev_factory, argc, argv, ARGP_IN_ORDER, &argc, args);

	free(argv[0]);
	argv[0] = argv0;
	state->next += argc - 1;
}

const char *argp_program_version = "1.0";
const char *argp_program_bug_address = "Matias Bjørling <mb@lightnvm.io>";
static char args_doc_global[] =
		"\nSupported commands are:\n"
		"  init         Initialize device for LightNVM\n"
		"  devices      List available LightNVM devices.\n"
		"  info         List general info and target engines\n"
		"  create       Create target on top of a specific device\n"
		"  remove       Remove target from device\n"
		"  factory      Reset device to factory state";

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
	struct arguments *args = state->input;

	switch (key) {
	case ARGP_KEY_ARG:
		if (strcmp(arg, "info") == 0)
			args->cmdtype = LIGHTNVM_INFO;
		else if (strcmp(arg, "devices") == 0)
			args->cmdtype = LIGHTNVM_DEVS;
		else if (strcmp(arg, "create") == 0) {
			args->cmdtype = LIGHTNVM_CREATE;
			cmd_create(state, args);
		} else if (strcmp(arg, "remove") == 0) {
			args->cmdtype = LIGHTNVM_REMOVE;
			cmd_remove(state, args);
		} else if (strcmp(arg, "init") == 0) {
			args->cmdtype = LIGHTNVM_DEV_INIT;
			cmd_dev_init(state, args);
		} else if (strcmp(arg, "factory") == 0) {
			args->cmdtype = LIGHTNVM_DEV_FACTORY;
			cmd_dev_factory(state, args);
		}
		break;
	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = {NULL, parse_opt, "[<cmd> [CMD-OPTIONS]]",
							args_doc_global};

int main(int argc, char **argv)
{
	char dev[FILENAME_MAX] = "/dev/lightnvm/control";
	int fd;
	struct arguments args = { 0 };

	argp_parse(&argp, argc, argv, ARGP_IN_ORDER, NULL, &args);

	fd = open(dev, O_WRONLY);
	if (fd < 0) {
		printf("Failed to open LightNVM mgmt %s. Error: %d\n",
								dev, fd);
		return -EINVAL;
	}

	switch (args.cmdtype) {
	case LIGHTNVM_INFO:
		list_info(fd);
		break;
	case LIGHTNVM_DEVS:
		list_devices(fd);
		break;
	case LIGHTNVM_CREATE:
		create_tgt(fd, &args);
		break;
	case LIGHTNVM_REMOVE:
		remove_tgt(fd, &args);
		break;
	case LIGHTNVM_DEV_INIT:
		dev_init(fd, &args);
		break;
	case LIGHTNVM_DEV_FACTORY:
		dev_factory(fd, &args);
		break;
	default:
		printf("No valid command given.\n");
	}

	close(fd);

	return 0;
}
