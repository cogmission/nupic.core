/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013-2016, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 * ----------------------------------------------------------------------
 */

/** @file
 * Definitions for the Extended Temporal Memory in C++
 */

#ifndef NTA_EXTENDED_TEMPORAL_MEMORY_HPP
#define NTA_EXTENDED_TEMPORAL_MEMORY_HPP

#include <vector>
#include <nupic/types/Serializable.hpp>
#include <nupic/types/Types.hpp>
#include <nupic/utils/Random.hpp>
#include <nupic/algorithms/Connections.hpp>

using namespace std;
using namespace nupic;
using namespace nupic::algorithms::connections;

#include <nupic/proto/ExtendedTemporalMemoryProto.capnp.h>

namespace nupic {
  namespace experimental {
    namespace extended_temporal_memory {

      /**
       * Extended Temporal Memory implementation in C++.
       *
       * The public API uses C arrays, not std::vectors, as inputs. C arrays are
       * a good lowest common denominator. You can get a C array from a vector,
       * but you can't get a vector from a C array without copying it. This is
       * important, for example, when using numpy arrays. The only way to
       * convert a numpy array into a std::vector is to copy it, but you can
       * access a numpy array's internal C array directly.
       */
      class ExtendedTemporalMemory :
        public Serializable<ExtendedTemporalMemoryProto> {
      public:
        ExtendedTemporalMemory();

        /**
         * Initialize the temporal memory (TM) using the given parameters.
         *
         * @param columnDimensions
         * Dimensions of the column space
         *
         * @param cellsPerColumn
         * Number of cells per column
         *
         * @param activationThreshold
         * If the number of active connected synapses on a segment is at least
         * this threshold, the segment is said to be active.
         *
         * @param initialPermanence
         * Initial permanence of a new synapse.
         *
         * @param connectedPermanence
         * If the permanence value for a synapse is greater than this value, it
         * is said to be connected.
         *
         * @param minThreshold
         * If the number of synapses active on a segment is at least this
         * threshold, it is selected as the best matching cell in a bursting
         * column.
         *
         * @param maxNewSynapseCount
         * The maximum number of synapses added to a segment during learning.
         *
         * @param permanenceIncrement
         * Amount by which permanences of synapses are incremented during
         * learning.
         *
         * @param permanenceDecrement
         * Amount by which permanences of synapses are decremented during
         * learning.
         *
         * @param predictedSegmentDecrement
         * Amount by which active permanences of synapses of previously
         * predicted but inactive segments are decremented.
         *
         * @param seed
         * Seed for the random number generator.
         *
         * @param maxSegmentsPerCell
         * The maximum number of segments per cell.
         *
         * @param maxSynapsesPerSegment
         * The maximum number of synapses per segment.
         *
         * Notes:
         *
         * predictedSegmentDecrement: A good value is just a bit larger than
         * (the column-level sparsity * permanenceIncrement). So, if column-level
         * sparsity is 2% and permanenceIncrement is 0.01, this parameter should be
         * something like 4% * 0.01 = 0.0004).
         */
        ExtendedTemporalMemory(
          vector<UInt> columnDimensions,
          UInt cellsPerColumn = 32,
          UInt activationThreshold = 13,
          Permanence initialPermanence = 0.21,
          Permanence connectedPermanence = 0.50,
          UInt minThreshold = 10,
          UInt maxNewSynapseCount = 20,
          Permanence permanenceIncrement = 0.10,
          Permanence permanenceDecrement = 0.10,
          Permanence predictedSegmentDecrement = 0.0,
          bool formInternalBasalConnections = true,
          bool learnOnOneCell = false,
          Int seed = 42,
          UInt maxSegmentsPerCell=255,
          UInt maxSynapsesPerSegment=255);

        virtual void initialize(
          vector<UInt> columnDimensions = { 2048 },
          UInt cellsPerColumn = 32,
          UInt activationThreshold = 13,
          Permanence initialPermanence = 0.21,
          Permanence connectedPermanence = 0.50,
          UInt minThreshold = 10,
          UInt maxNewSynapseCount = 20,
          Permanence permanenceIncrement = 0.10,
          Permanence permanenceDecrement = 0.10,
          Permanence predictedSegmentDecrement = 0.0,
          bool formInternalBasalConnections = true,
          bool learnOnOneCell = false,
          Int seed = 42,
          UInt maxSegmentsPerCell=255,
          UInt maxSynapsesPerSegment=255);

        virtual ~ExtendedTemporalMemory();

        //----------------------------------------------------------------------
        //  Main functions
        //----------------------------------------------------------------------

        /**
         * Get the version number of for the TM implementation.
         *
         * @returns Integer version number.
         */
        virtual UInt version() const;

        /**
         * This *only* updates _rng to a new Random using seed.
         *
         * @returns Integer version number.
         */
        void seed_(UInt64 seed);

        /**
         * Indicates the start of a new sequence.
         * Resets sequence state of the TM.
         */
        virtual void reset();

        /**
         * Calculate the active cells, using the current active columns and
         * dendrite segments. Grow and reinforce synapses.
         *
         * @param activeColumnsSize
         * Size of activeColumns.
         *
         * @param activeColumns
         * A sorted list of active column indices.
         *
         * @param prevActiveExternalCellsBasalSize
         * Size of prevActiveExternalCellsBasal.
         *
         * @param prevActiveExternalCellsBasal
         * A sorted list of the external cells that were used to calculate the
         * current basal segment excitation. This class doesn't save a copy of
         * these cells because the caller is more flexible to find ways of
         * keeping this list available without extra copying.
         *
         * @param prevActiveExternalCellsApicalSize
         * Size of prevActiveExternalCellsApical.
         *
         * @param prevActiveExternalCellsApical
         * A sorted list of the external cells that were used to calculate the
         * current apical segment excitation. This class doesn't save a copy of
         * these cells because the caller is more flexible to find ways of
         * keeping this list available without extra copying.
         *
         * @param learn
         * If true, reinforce / punish / grow synapses.
         */
        void activateCells(
          size_t activeColumnsSize,
          const UInt activeColumns[],

          size_t prevActiveExternalCellsBasalSize,
          const CellIdx prevActiveExternalCellsBasal[],

          size_t activeExternalCellsApicalSize,
          const CellIdx activeExternalCellsApical[],

          bool learn);

        /**
         * Calculate dendrite segment activity, using the current active cells.
         *
         * @param activeExternalCellsBasalSize
         * Size of activeExternalCellsBasal.
         *
         * @param activeExternalCellsBasal
         * Sorted list of active external cells for activating basal dendrites.
         *
         * @param activeExternalCellsApicalSize
         * Size of activeExternalCellsApical.
         *
         * @param activeExternalCellsApical
         * Sorted list of active external cells for activating apical dendrites.
         *
         * @param learn
         * If true, segment activations will be recorded. This information is
         * used during segment cleanup.
         */
        void activateDendrites(
          size_t activeExternalCellsBasalSize,
          const CellIdx activeExternalCellsBasal[],

          size_t activeExternalCellsApicalSize,
          const CellIdx activeExternalCellsApical[],

          bool learn = true);

        /**
         * Perform one time step of the Temporal Memory algorithm.
         *
         * This method calls activateCells, then calls activateDendrites. Using
         * the TemporalMemory via its compute method ensures that you'll always
         * be able to call getPredictiveCells to get predictions for the next
         * time step.
         *
         * @param activeColumnsSize
         * Size of activeColumns.
         *
         * @param activeColumns
         * Sorted list of indices of active columns.
         *
         * @param prevActiveExternalCellsBasalSize
         * Size of prevActiveExternalCellsBasal.
         *
         * @param prevActiveExternalCellsBasal
         * The external cells that were used to calculate the current basal
         * segment excitation. This class doesn't save a copy of these cells
         * because the caller is more flexible to find ways of keeping this list
         * available without extra copying.
         *
         * @param activeExternalCellsBasalSize
         * Size of activeExternalCellsBasal.
         *
         * @param activeExternalCellsBasal
         * Sorted list of active external cells that should be used for
         * activating basal dendrites in this timestep.
         *
         * @param prevActiveExternalCellsApicalSize
         * Size of prevActiveExternalCellsApical.
         *
         * @param prevActiveExternalCellsApical
         * The external cells that were used to calculate the current apical
         * segment excitation. This class doesn't save a copy of these cells
         * because the caller is more flexible to find ways of keeping this list
         * available without extra copying.
         *
         * @param activeExternalCellsApicalSize
         * Size of activeExternalCellsApical.
         *
         * @param activeExternalCellsApical
         * Sorted list of active external cells that should be used for
         * activating apical dendrites in this timestep.
         *
         * @param learn
         * Whether or not learning is enabled.
         */
        virtual void compute(
          size_t activeColumnsSize,
          const UInt activeColumns[],

          size_t prevActiveExternalCellsBasalSize = 0,
          const CellIdx prevActiveExternalCellsBasal[] = nullptr,

          size_t activeExternalCellsBasalSize = 0,
          const CellIdx activeExternalCellsBasal[] = nullptr,

          size_t prevActiveExternalCellsApicalSize = 0,
          const CellIdx prevActiveExternalCellsApical[] = nullptr,

          size_t activeExternalCellsApicalSize = 0,
          const CellIdx activeExternalCellsApical[] = nullptr,

          bool learn = true);

        // ==============================
        //  Helper functions
        // ==============================

        /**
         * Returns the indices of cells that belong to a column.
         *
         * @param column Column index
         *
         * @return (vector<CellIdx>) Cell indices
         */
        vector<CellIdx> cellsForColumn(Int column);

        /**
         * Returns the number of cells in this layer.
         *
         * @return (int) Number of cells
         */
        UInt numberOfCells(void);

        /**
        * Returns the indices of the active cells.
        *
        * @returns (std::vector<CellIdx>) Vector of indices of active cells.
        */
        vector<CellIdx> getActiveCells() const;

        /**
        * Returns the indices of the predictive cells.
        *
        * @returns (std::vector<CellIdx>) Vector of indices of predictive cells.
        */
        vector<CellIdx> getPredictiveCells() const;

        /**
        * Returns the indices of the winner cells.
        *
        * @returns (std::vector<CellIdx>) Vector of indices of winner cells.
        */
        vector<CellIdx> getWinnerCells() const;

        vector<Segment> getActiveBasalSegments() const;
        vector<Segment> getMatchingBasalSegments() const;
        vector<Segment> getActiveApicalSegments() const;
        vector<Segment> getMatchingApicalSegments() const;

        /**
         * Returns the dimensions of the columns in the region.
         *
         * @returns Integer number of column dimension
         */
        vector<UInt> getColumnDimensions() const;

        /**
         * Returns the total number of columns.
         *
         * @returns Integer number of column numbers
         */
        UInt numberOfColumns() const;

        /**
         * Returns the number of cells per column.
         *
         * @returns Integer number of cells per column
         */
        UInt getCellsPerColumn() const;

        /**
         * Returns the activation threshold.
         *
         * @returns Integer number of the activation threshold
         */
        UInt getActivationThreshold() const;
        void setActivationThreshold(UInt);

        /**
         * Returns the initial permanence.
         *
         * @returns Initial permanence
         */
        Permanence getInitialPermanence() const;
        void setInitialPermanence(Permanence);

        /**
         * Returns the connected permanance.
         *
         * @returns Returns the connected permanance
         */
        Permanence getConnectedPermanence() const;
        void setConnectedPermanence(Permanence);

        /**
         * Returns the minimum threshold.
         *
         * @returns Integer number of minimum threshold
         */
        UInt getMinThreshold() const;
        void setMinThreshold(UInt);

        /**
         * Returns the maximum new synapse count.
         *
         * @returns Integer number of maximum new synapse count
         */
        UInt getMaxNewSynapseCount() const;
        void setMaxNewSynapseCount(UInt);

        /**
         * Returns whether to form internal connections between cells.
         *
         * @returns the formInternalBasalConnections parameter
         */
        bool getFormInternalBasalConnections() const;
        void setFormInternalBasalConnections(bool formInternalBasalConnections);

        /**
         * Returns whether to always choose the same cell when bursting a column
         * until the next reset occurs.
         *
         * @returns the learnOnOneCell parameter
         */
        bool getLearnOnOneCell() const;
        void setLearnOnOneCell(bool learnOnOneCell);

        /**
         * Returns the permanence increment.
         *
         * @returns Returns the Permanence increment
         */
        Permanence getPermanenceIncrement() const;
        void setPermanenceIncrement(Permanence);

        /**
         * Returns the permanence decrement.
         *
         * @returns Returns the Permanence decrement
         */
        Permanence getPermanenceDecrement() const;
        void setPermanenceDecrement(Permanence);

        /**
         * Returns the predicted Segment decrement.
         *
         * @returns Returns the segment decrement
         */
        Permanence getPredictedSegmentDecrement() const;
        void setPredictedSegmentDecrement(Permanence);

        /**
         * Raises an error if cell index is invalid.
         *
         * @param cell Cell index
         */
        bool _validateCell(CellIdx cell);

        /**
         * Save (serialize) the current state of the spatial pooler to the
         * specified file.
         *
         * @param fd A valid file descriptor.
         */
        virtual void save(ostream& outStream) const;

        using Serializable::write;
        virtual void write(
          ExtendedTemporalMemoryProto::Builder& proto) const override;

        /**
         * Load (deserialize) and initialize the spatial pooler from the
         * specified input stream.
         *
         * @param inStream A valid istream.
         */
        virtual void load(istream& inStream);

        using Serializable::read;
        virtual void read(ExtendedTemporalMemoryProto::Reader& proto) override;

        /**
         * Returns the number of bytes that a save operation would result in.
         * Note: this method is currently somewhat inefficient as it just does
         * a full save into an ostream and counts the resulting size.
         *
         * @returns Integer number of bytes
         */
        virtual UInt persistentSize() const;

        //----------------------------------------------------------------------
        // Debugging helpers
        //----------------------------------------------------------------------

        /**
         * Print the main TM creation parameters
         */
        void printParameters();

        /**
         * Returns the index of the column that a cell belongs to.
         *
         * @param cell Cell index
         *
         * @return (int) Column index
         */
        Int columnForCell(CellIdx cell);

        /**
         * Print the given UInt array in a nice format
         */
        void printState(vector<UInt> &state);

        /**
         * Print the given Real array in a nice format
         */
        void printState(vector<Real> &state);

      protected:
        UInt numColumns_;
        vector<UInt> columnDimensions_;
        UInt cellsPerColumn_;
        UInt activationThreshold_;
        UInt minThreshold_;
        UInt maxNewSynapseCount_;
        bool formInternalBasalConnections_;
        Permanence initialPermanence_;
        Permanence connectedPermanence_;
        Permanence permanenceIncrement_;
        Permanence permanenceDecrement_;
        Permanence predictedSegmentDecrement_;

        vector<CellIdx> activeCells_;
        vector<CellIdx> winnerCells_;

        vector<Segment> activeBasalSegments_;
        vector<Segment> matchingBasalSegments_;
        vector<UInt32> numActiveConnectedSynapsesForBasalSegment_;
        vector<UInt32> numActivePotentialSynapsesForBasalSegment_;

        vector<Segment> activeApicalSegments_;
        vector<Segment> matchingApicalSegments_;
        vector<UInt32> numActiveConnectedSynapsesForApicalSegment_;
        vector<UInt32> numActivePotentialSynapsesForApicalSegment_;

        bool learnOnOneCell_;
        map<UInt, CellIdx> chosenCellForColumn_;

        Random rng_;

      public:
        Connections basalConnections;
        Connections apicalConnections;
      };

    } // end namespace extended_temporal_memory
  } // end namespace algorithms
} // end namespace nupic

#endif // NTA_EXTENDED_TEMPORAL_MEMORY_HPP
