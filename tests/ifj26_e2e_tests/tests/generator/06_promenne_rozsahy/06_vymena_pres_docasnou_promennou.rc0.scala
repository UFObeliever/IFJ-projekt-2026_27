import ifj26.*

@main def main(): Unit =
  var a = 1
  var b = 2
  val t = a
  a = b
  b = t
  print(a)
  print("\n")
  print(b)
  print("\n")
