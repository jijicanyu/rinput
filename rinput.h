/* rinput.h
 * Heiher <admin@heiher.info>
 */

#ifndef __RINPUT_H__
#define __RINPUT_H__

#include <stdint.h>

struct rinput_event
{
	uint16_t type;
	uint16_t code;
	int32_t value;
};

#endif /* __RINPUT_H__ */

