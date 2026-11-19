import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

def add(a: Int, b: Int): Int = a + b

@main
def main(): Unit =
  val r = add(tr("a", 1) + tr("b", 1), tr("c", 1) + tr("d", 1))
  print("|" + toStr(r))
