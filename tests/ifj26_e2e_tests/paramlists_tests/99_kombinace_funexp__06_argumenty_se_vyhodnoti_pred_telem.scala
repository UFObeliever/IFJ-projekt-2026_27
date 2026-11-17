import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

def g(a: Int)(b: Int): Int =
  print("B")
  a + b

@main
def main(): Unit =
  val r = g(tr("a", 1))(tr("b", 2))
  print("|" + toStr(r))
