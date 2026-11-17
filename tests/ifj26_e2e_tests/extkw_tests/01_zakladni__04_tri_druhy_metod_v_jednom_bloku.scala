import ifj26.*

extension (value: Int)
  // bezparametrická metoda
  def twice: Int = value * 2
  // metoda s prázdným seznamem parametrů
  def foo(): String =
    val s = toStr(value)
    "foo" + s
  // metoda s jedním parametrem
  def plus(other: Int): Int = value + other

@main
def main(): Unit =
  val x: Int = 6
  val y = x.twice
  val z = x.foo()
  val w = x.plus(12)
  print(y)
  print(",")
  print(z)
  print(",")
  print(w)
