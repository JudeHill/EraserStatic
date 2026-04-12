

int main(char* name)
{
   char* val;

   for (val = ""; *val == '\0';) {
      val = getparam(name);
   }
   return (atoi(val));
}