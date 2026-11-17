import ifj26.*

extension (s: String)
  def doubled: Int = s.length * 2

@main
def main(): Unit =
  val n = "abcd".doubled
  print(n)
