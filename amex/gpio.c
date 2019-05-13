#include <windows.h>
#include <ntstatus.h>

#include <assert.h>
#include <string.h>

#include "amex/gpio.h"

#include "hook/iohook.h"

#include "util/dprintf.h"
#include "util/setupapi.h"
#include "util/str.h"

enum {
    GPIO_IOCTL_SET_LEDS     = 0x8000A004,
    GPIO_IOCTL_GET_PSW      = 0x80006008,
    GPIO_IOCTL_GET_DIPSW    = 0x8000600C,
    GPIO_IOCTL_DESCRIBE     = 0x80006014,
};

enum {
    GPIO_TYPE_NONE  = 0,
    GPIO_TYPE_LED   = 1,
    GPIO_TYPE_DIPSW = 2,
    GPIO_TYPE_PSW   = 3,
};

#pragma pack(push, 1)

struct gpio_port {
    uint8_t unknown;

    /* Number of distinct instances of this thing..? */
    uint8_t count;

    /* Type of GPIO port */
    uint16_t type;
};

struct gpio_ports {
    uint8_t unknown; /* Maybe a count of valid items in the array idk */
    struct gpio_port ports[32];
};

#pragma pack(pop)

static HRESULT gpio_handle_irp(struct irp *irp);
static HRESULT gpio_handle_open(struct irp *irp);
static HRESULT gpio_handle_close(struct irp *irp);
static HRESULT gpio_handle_ioctl(struct irp *irp);

static HRESULT gpio_ioctl_get_psw(struct irp *irp);
static HRESULT gpio_ioctl_get_dipsw(struct irp *irp);
static HRESULT gpio_ioctl_describe(struct irp *irp);
static HRESULT gpio_ioctl_set_leds(struct irp *irp);

static const struct gpio_ports gpio_ports = {
    .ports = {
        {
            .type       = GPIO_TYPE_LED,
            .count      = 2,
        }, {
            .type       = GPIO_TYPE_DIPSW,
            .count      = 8,
        }, {
            .type       = GPIO_TYPE_PSW,
            .count      = 2,
        }
    },
};

static_assert(sizeof(gpio_ports) == 129, "GPIO port map size");

static HANDLE gpio_fd;
static struct gpio_config gpio_config;

HRESULT gpio_hook_init(const struct gpio_config *cfg)
{
    HRESULT hr;

    assert(cfg != NULL);

    if (!cfg->enable) {
        return S_FALSE;
    }

    memcpy(&gpio_config, cfg, sizeof(*cfg));

    gpio_fd = iohook_open_dummy_fd();
    hr = iohook_push_handler(gpio_handle_irp);

    if (FAILED(hr)) {
        return hr;
    }

    hr = setupapi_add_phantom_dev(&gpio_guid, L"$gpio");

    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

static HRESULT gpio_handle_irp(struct irp *irp)
{
    assert(irp != NULL);

    if (irp->op != IRP_OP_OPEN && irp->fd != gpio_fd) {
        return iohook_invoke_next(irp);
    }

    switch (irp->op) {
    case IRP_OP_OPEN:   return gpio_handle_open(irp);
    case IRP_OP_CLOSE:  return gpio_handle_close(irp);
    case IRP_OP_IOCTL:  return gpio_handle_ioctl(irp);
    default:            return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT gpio_handle_open(struct irp *irp)
{
    if (!wstr_eq(irp->open_filename, L"$gpio")) {
        return iohook_invoke_next(irp);
    }

    dprintf("GPIO: Open device\n");
    irp->fd = gpio_fd;

    return S_OK;
}

static HRESULT gpio_handle_close(struct irp *irp)
{
    dprintf("GPIO: Close device\n");

    return S_OK;
}

static HRESULT gpio_handle_ioctl(struct irp *irp)
{
    switch (irp->ioctl) {
    case GPIO_IOCTL_SET_LEDS:
        return gpio_ioctl_set_leds(irp);

    case GPIO_IOCTL_GET_PSW:
        return gpio_ioctl_get_psw(irp);

    case GPIO_IOCTL_GET_DIPSW:
        return gpio_ioctl_get_dipsw(irp);

    case GPIO_IOCTL_DESCRIBE:
        return gpio_ioctl_describe(irp);

    default:
        dprintf("GPIO: Unknown ioctl %08x, write %i read %i\n",
                irp->ioctl,
                (int) irp->write.nbytes,
                (int) irp->read.nbytes);

        return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }
}

static HRESULT gpio_ioctl_get_dipsw(struct irp *irp)
{
    uint32_t dipsw;
    size_t i;

    dipsw = 0;

    for (i = 0 ; i < 8 ; i++) {
        if (gpio_config.dipsw[i]) {
            dipsw |= 1 << i;
        }
    }

    //dprintf("GPIO: Get dipsw %08x\n", dipsw);

    return iobuf_write_le32(&irp->read, dipsw);
}

static HRESULT gpio_ioctl_get_psw(struct irp *irp)
{
    uint32_t result;

    result = 0;

    /* Bit 0 == SW1 == Alt. Test */
    /* Bit 1 == SW2 == Alt. Service */

    if (gpio_config.vk_sw1 && (GetAsyncKeyState(gpio_config.vk_sw1) & 0x8000)) {
        result |= 1 << 0;
    }

    if (gpio_config.vk_sw2 && (GetAsyncKeyState(gpio_config.vk_sw2) & 0x8000)) {
        result |= 1 << 1;
    }

    return iobuf_write_le32(&irp->read, result);
}

static HRESULT gpio_ioctl_describe(struct irp *irp)
{
    dprintf("GPIO: Describe GPIO ports\n");

    if (irp->read.nbytes < sizeof(gpio_ports)) {
        dprintf("GPIO: Descriptor read buffer too small\n");

        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    memcpy(irp->read.bytes, &gpio_ports, sizeof(gpio_ports));
    irp->read.pos = sizeof(gpio_ports);

    return S_OK;
}

static HRESULT gpio_ioctl_set_leds(struct irp *irp)
{
    return S_OK;
}
