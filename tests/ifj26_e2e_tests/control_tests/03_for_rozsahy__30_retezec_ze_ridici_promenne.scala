import ifj26.*

@main
def main(): Unit =
  var s = ""
  for i <- 1 to 3 do
    val t = toStr(i)
    s = s + t
  print(s)
