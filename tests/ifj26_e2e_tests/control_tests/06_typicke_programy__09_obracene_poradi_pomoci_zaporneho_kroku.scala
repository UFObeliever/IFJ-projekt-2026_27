import ifj26.*

@main
def main(): Unit =
  var s = ""
  for i <- 5 to 1 by -1 do
    val t = toStr(i)
    s = s + t
  print(s)
