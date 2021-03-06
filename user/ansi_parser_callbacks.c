/**
 * ANSI code parser callbacks that do the actual work.
 * Split from the parser itself for easier editing without
 * having to deal with Ragel.
 */

#include <esp8266.h>
#include <helpers.h>
#include "ansi_parser_callbacks.h"
#include "cgi_sockets.h"
#include "version.h"
#include "uart_buffer.h"
#include "screen.h"
#include "wifimgr.h"

volatile bool enquiry_suppressed = false;
ETSTimer enqTimer;
void ICACHE_FLASH_ATTR enqTimerCb(void *unused)
{
	enquiry_suppressed = false;
}

/**
 * Send a response to UART0
 * @param str
 */
void ICACHE_FLASH_ATTR
apars_respond(const char *str)
{
	// Using the Tx buffer causes issues with large data (eg. from http requests)
	UART_SendAsync(str, -1);
}

/**
 * Beep at the user.
 */
void ICACHE_FLASH_ATTR
apars_handle_bel(void)
{
	send_beep();
}

/**
 * Send to uart the answerback message
 */
void ICACHE_FLASH_ATTR
apars_handle_enq(void)
{
	if (enquiry_suppressed) return;

	u8 mac[6];
	wifi_get_macaddr(SOFTAP_IF, mac);

	char buf100[100];
	char *buf = buf100;
	buf += sprintf(buf, "\x1bX");
	buf += sprintf(buf, "ESPTerm "VERSION_STRING" ");
	buf += sprintf(buf, "#"GIT_HASH_BACKEND"+"GIT_HASH_FRONTEND" ");
	buf += sprintf(buf, "id=%02X%02X%02X ", mac[3], mac[4], mac[5]);
	int x = getStaIpAsString(buf);
	if (x) buf += x;
	else buf--; // remove the trailing space
	buf += sprintf(buf, "\x1b\\");

	(void)buf;

	// version encased in SOS and ST
	apars_respond(buf100);

	// Throttle enquiry - this is a single-character-invoked response,
	// so it tends to happen randomly when throwing garbage at the ESP.
	// We don't want to fill the output buffer with dozens of enquiry responses
	enquiry_suppressed = true;
	TIMER_START(&enqTimer, enqTimerCb, 500, 0);
}

void ICACHE_FLASH_ATTR
apars_handle_tab(void)
{
	screen_tab_forward(1);
}
