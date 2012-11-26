// Stub for DOS/Linux/Win32 to allow main() on embedded targets to spin in an
// infinite loop once tests are complete.

int t_all( void);
int main( int argc, char **argv)
{
	return t_all();
}
