import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

extension (value: Int)
  def twice: Int = value * 2
  def foo(): Int = value + 1
  def plus(other: Int): Int = value + other

@main
def main(): Unit =
  val r = tr("r", 1).plus(tr("a", 2))
  print("|")
  print(r)
