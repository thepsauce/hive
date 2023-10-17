/* this is the interface to connect chat and hive, hc stands for hive-chat */

typedef struct hc {
	Hive hive;
	NetChat chat;
} HiveChat;

void hc_init(HiveChat *hc);

extern HiveChat hive_chat;

/* the aditional pointer parameter is to identify the specific
 * hive-chat connection but since there is just one, this pointer
 * is unused
 */
/* a move is in the simple format:
 * [0|1] [x position],[y position] [x position],[y position] 
 */
/* send a notification to the server */
int hc_notifymove(void *ptr, const HiveMove *move);
/* used when a notification was received */
int hc_domove(void *ptr, const char *move);