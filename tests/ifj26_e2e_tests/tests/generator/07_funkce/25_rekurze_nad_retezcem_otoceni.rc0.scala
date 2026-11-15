import ifj26.*

def rev(s: String): String =
  val n = strLength(s)
  if n < 2 then
    return s
  else
    val first = subStr(s, 0, 1)
    val rest = subStr(s, 1, n)
    val r = rev(rest)
    return r + first

@main def main(): Unit =
  val r = rev("stressed")
  print(r)
  print("\n")
