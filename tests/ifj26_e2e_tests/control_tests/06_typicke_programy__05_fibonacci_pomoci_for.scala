import ifj26.*

@main
def main(): Unit =
  var a = 0
  var b = 1
  for i <- 1 to 10 do
    val t = a + b
    a = b
    b = t
  print(a)
