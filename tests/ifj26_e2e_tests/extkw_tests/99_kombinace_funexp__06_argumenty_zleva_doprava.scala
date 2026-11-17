import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

extension (v: Int)
  def span(lo: Int, hi: Int): Int = hi - lo + v

@main
def main(): Unit =
  val x = 1
  val r = x.span(tr("a", 0), tr("b", 10))
  print("|")
  print(r)
