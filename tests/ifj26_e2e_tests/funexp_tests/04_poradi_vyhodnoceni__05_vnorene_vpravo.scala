import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

def add(a: Int, b: Int): Int = a + b

@main
def main(): Unit =
  val r = add(tr("a", 1), add(tr("b", 2), tr("c", 3)))
  print("|" + toStr(r))
