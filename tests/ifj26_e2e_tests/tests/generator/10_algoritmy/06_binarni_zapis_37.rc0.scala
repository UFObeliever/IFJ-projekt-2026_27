import ifj26.*

@main def main(): Unit =
  var n = 37
  var s = ""
  while n > 0 do
    val bit = n - n / 2 * 2
    val b = toStr(bit)
    s = b + s
    n = n / 2
  print(s)
  print("\n")
