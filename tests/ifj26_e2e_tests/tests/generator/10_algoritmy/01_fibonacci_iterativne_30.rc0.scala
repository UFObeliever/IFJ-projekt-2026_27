import ifj26.*

@main def main(): Unit =
  var a = 0
  var b = 1
  var i = 0
  while i < 30 do
    val t = a + b
    a = b
    b = t
    i = i + 1
  print(a)
  print("\n")
