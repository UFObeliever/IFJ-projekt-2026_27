import ifj26.*

@main def main(): Unit =
  var x = 1.0
  var i = 0
  while i < 10 do
    x = (x + 2.0 / x) / 2.0
    i = i + 1
  val t = toStr(x)
  print(t)
  print("\n")
