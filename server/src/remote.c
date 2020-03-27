/*
 * Server functions to handle telnet connections to the remote client.
 *
 * The server generates a SecureKey, that allow the remote cittaclients
 * to identify as such, then forks a cittad daemon. The daemon listens to the
 * PORT_REMOTE port for incoming connections and starts the remote_cittaclient
 * with the '-k SecureKey' option for each new telnet connection.
 */

#define SECURE_KEY_LEN 16
void generate_secure_key(void)
{

}
