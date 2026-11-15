import ifj26.*

@main def main(): Unit =
  var s = 0
  var x = readInt()
  while x != 0 do
    s = s + x
    x = readInt()
  print(s)
  print("\n")
