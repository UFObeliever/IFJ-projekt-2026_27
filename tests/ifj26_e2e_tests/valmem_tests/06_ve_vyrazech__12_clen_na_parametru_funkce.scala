import ifj26.*

def cnt(s: String, d: Double): Int =
  val a = s.length
  val b = d.toInt
  a + b

@main
def main(): Unit =
  val r = cnt("abcd", 2.9)
  print(r)
