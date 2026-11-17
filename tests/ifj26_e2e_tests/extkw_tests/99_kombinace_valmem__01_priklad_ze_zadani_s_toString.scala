import ifj26.*

extension (value: Int)
  // bezparametrická metoda
  def twice: Int = value * 2
  // metoda s prázdným seznamem parametrů
  def foo(): String = "foo" + value.toString
  // metoda s jedním parametrem
  def plus(other: Int): Int = value + other

def someFn(): Unit =
  val x: Int = 6
  val y = x.twice
  val z = x.foo()
  val w = x.plus(12)
  print(y)
  print(z)
  print(w)

@main
def main(): Unit =
  someFn()
