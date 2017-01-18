{-# LANGUAGE OverloadedStrings #-}
module Main
       where

import Control.Applicative
import Control.Monad
import qualified Data.ByteString.Char8 as BS
import qualified Data.HashMap.Strict as HM
import Data.List
import Data.Maybe
import System.Environment
import System.FilePath
import System.IO

main :: IO ()
main = getArgs >>= mainWithArgs
  where mainWithArgs [ bomIn ] = fixupBom bomIn
        mainWithArgs [ ] = do prog <- getProgName
                              hPutStrLn stderr $ "Usage: " ++ prog ++ " <BOM.csv>\n       " ++ prog ++ " <OUT> <BOM1.csv> <BOM2.csv> ..."
        mainWithArgs (out:bomsIn) = fixupBoms out bomsIn

fixupBom :: FilePath -> IO ()
fixupBom bomInFile = do bom <- liftM parseBom $ BS.readFile bomInFile
                        BS.writeFile (bomOrderFile bomInFile) $ bomOrder bom
                        BS.writeFile (bomAssemblyFile bomInFile) $ bomAssembly bom

fixupBoms :: FilePath -> [FilePath] -> IO ()
fixupBoms outbase bomsIn = do boms <- mapM (liftM parseBom . BS.readFile) bomsIn
                              let bom = concat boms
                              BS.writeFile (bomOrderFile outbase) $ bomOrder bom
                              BS.writeFile (bomAssemblyFile outbase) $ bomAssembly bom

bomOrderFile :: FilePath -> FilePath
bomOrderFile base = (dropExtension base) ++ "-order.txt"

bomAssemblyFile :: FilePath -> FilePath
bomAssemblyFile base = (dropExtension base) ++ "-assembly.txt"

bomOrder :: [Bom] -> BS.ByteString
bomOrder bom = BS.unlines . map pnoLine $ bomByPno 
  where bomByPno = HM.elems $ HM.fromListWith (++) [ (bomVendorPno b, [b]) | b <- bom ]
        pnoLine bs@(b0:_) = BS.intercalate "\t" [ BS.pack . show . length $ bs
                                                , bomVendorPno b0
                                                , BS.intercalate ";" . map bomPart $ bs
                                                ]

bomAssembly :: [Bom] -> BS.ByteString
bomAssembly bom = BS.unlines . map bomLine $ bom
  where bomLine b = BS.intercalate "\t" [ bomPart b
                                        , bomValue b
                                        , bomVendorPno b
                                        , fromMaybe "" $ bomManf b
                                        , fromMaybe "" $ bomManfPno b
                                        ]

data Bom = Bom { bomPart :: !BS.ByteString
               , bomValue :: !BS.ByteString
               , bomVendorPno :: !BS.ByteString
               , bomManf :: !(Maybe BS.ByteString)
               , bomManfPno :: !(Maybe BS.ByteString)
               } deriving (Show)

vendorName = "DIGIKEY"

parseBom :: BS.ByteString -> [Bom]
parseBom eaglebom = concatMap (parseBomLine fields) parts
  where (header:parts) = BS.lines eaglebom
        fields = tokenizeBomLine header
        
parseBomLine :: [BS.ByteString] -> BS.ByteString -> [Bom]
parseBomLine fields l = filter (not . BS.null . bomVendorPno) $ mapMaybe parseBomEntry [ "", "2", "3", "4", "5", "6", "7", "8", "9" ]
  where parseBomEntry suffix = Bom <$>
                               getField partField <*>
                               getField valueField <*>
                               getField vendorPnoField <*>
                               (pure $ getField manfField) <*>
                               (pure $ getField manfPnoField)
          where values = tokenizeBomLine l
                getField n = liftM snd . find ((== n) . fst) $ zip fields values
                partField = "Part"
                valueField = "Value"
                vendorPnoField = BS.concat [ vendorName, suffix, "#" ]
                manfField = BS.concat [ "MANF", suffix ]
                manfPnoField = BS.concat [ "MANF", suffix, "#" ]

tokenizeBomLine :: BS.ByteString -> [BS.ByteString]
tokenizeBomLine l = mapMaybe dropQuotes . BS.split ';' $ l
  where dropQuotes str = case BS.uncons str of
          Nothing -> Nothing
          Just ('"', rest) -> case BS.unsnoc rest of
            Nothing -> error $ "Malformed field in " ++ show l
            Just (res, '"') -> Just res
            Just _ -> error $ "Malformed field " ++ show str ++ " in " ++ show l
          Just _ -> Just str
                         
