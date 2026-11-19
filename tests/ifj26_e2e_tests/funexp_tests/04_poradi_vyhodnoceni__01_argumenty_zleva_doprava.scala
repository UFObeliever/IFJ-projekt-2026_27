import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

def add3(a: Int, b: Int, c: Int): Int = a + b + c

@main
def main(): Unit =
  val r = add3(tr("a", 1), tr("b", 2), tr("c", 3))
  print("|" + toStr(r))
