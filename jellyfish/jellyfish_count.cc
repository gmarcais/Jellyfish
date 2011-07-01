int count_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  return count_main(argc - 1, argv + 1);
}
