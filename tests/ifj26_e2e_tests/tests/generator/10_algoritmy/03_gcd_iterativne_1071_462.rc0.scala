import ifj26.*

@main def main(): Unit =
  var a = 1071
  var b = 462
  while b != 0 do
    val t = b
    b = a - a / b * b
    a = t
  print(a)
  print("\n")
