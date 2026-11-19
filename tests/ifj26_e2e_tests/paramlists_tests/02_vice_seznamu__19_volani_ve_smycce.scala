import ifj26.*

def add2(a: Int)(b: Int): Int = a + b

@main
def main(): Unit =
  var i = 1
  var s = 0
  while i <= 4 do
    val t = add2(i)(i)
    s = s + t
    i = i + 1
  print(s)
