#ifndef TEXTPARSER_HH
#define TEXTPARSER_HH

struct TextBaseEntry
{
  string address;
  string password;
  unsigned int quotaKB;
  string forward;
};
void textCallback(const TextBaseEntry &entry);
#endif /* TEXTPARSER_HH */
