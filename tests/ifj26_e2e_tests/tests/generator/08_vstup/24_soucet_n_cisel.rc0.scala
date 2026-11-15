import ifj26.*

@main def main(): Unit =
  val n = readInt()
  var i = 0
  var s = 0
  while i < n do
    val x = readInt()
    s = s + x
    i = i + 1
  print(s)
  print("\n")
