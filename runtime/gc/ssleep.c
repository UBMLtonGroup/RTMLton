/* signal free sleep */
int ssleep(int s, int us)
{
	int ret = 0;
	struct timeval tv;

	if (s == 0 && us == 0) s = 1;

	tv.tv_sec = s;
	tv.tv_usec = us;

	do
	{
	  ret = select(1, NULL, NULL, NULL, &tv);
	}
	while((ret == -1)&&(errno == EINTR));

	return ret;
}
