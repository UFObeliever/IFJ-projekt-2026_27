import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

@main
def main(): Unit =
  tr("a", 1)
  val r = tr("b", 2) + 1
  print("|" + toStr(r))
