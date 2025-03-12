#ifdef _WIN32
# define IS_SEP(c)  (c == '\\' || c == '/')
#else
# define IS_SEP(c)  (c == '/')
#endif

/* get basename */
const char *simple_basename(const char *str)
{
    for (const char *p = str; *p != 0; p++) {
        if (IS_SEP(*p) && *(p+1) != 0) {
            str = p + 1;
        }
    }

    return str;
}
