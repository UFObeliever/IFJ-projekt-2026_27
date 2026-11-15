import ifj26.*

def sq(a: Int): Int = a * a

@main def main(): Unit =
  var i = 1
  var s = 0
  while i <= 5 do
    val r = sq(i)
    s = s + r
    i = i + 1
  print(s)
  print("\n")
