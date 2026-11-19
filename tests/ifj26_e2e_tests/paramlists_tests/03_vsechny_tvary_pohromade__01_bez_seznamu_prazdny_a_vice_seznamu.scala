import ifj26.*

def p: Int = 1
def e(): Int = 2
def m(a: Int)(b: Int): Int = a + b
def s(a: Int): Int = a * 10

@main
def main(): Unit =
  val a = p
  val b = e()
  val c = m(a)(b)
  val d = s(c)
  print(d)
