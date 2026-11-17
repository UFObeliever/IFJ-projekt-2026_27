import ifj26.*

def inc(a: Int)(b: Int): Int =
  var t = a
  t = t + b
  t

@main
def main(): Unit =
  val r = inc(5)(6)
  print(r)
