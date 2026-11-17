import ifj26.*

extension (v: Int)
  def describe: String = "int"

extension (v: Double)
  def describe: String = "double"

extension (v: String)
  def describe: String = "string"

@main
def main(): Unit =
  val a = 1
  val b = 1.5
  val c = "x"
  val r1 = a.describe
  val r2 = b.describe
  val r3 = c.describe
  print(r1)
  print(",")
  print(r2)
  print(",")
  print(r3)
