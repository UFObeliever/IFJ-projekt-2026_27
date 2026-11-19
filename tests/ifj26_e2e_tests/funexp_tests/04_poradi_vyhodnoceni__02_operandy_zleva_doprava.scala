import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

@main
def main(): Unit =
  val r = tr("a", 1) + tr("b", 2) * tr("c", 3)
  print("|" + toStr(r))
